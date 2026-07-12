#include "openvic-simulation/ecs/World.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <thread>
#include <utility>

#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/SystemScheduler.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic::ecs;

World::World() = default;

bool World::in_tick_or_log_(char const* fn_name) {
	// During CommandBuffer::apply the scheduler sets in_apply_phase_ so the ops queued
	// via ctx.cmd actually execute their structural mutations. Outside of apply,
	// in_tick_ still blocks direct mutations during a tick.
	if (in_tick_ && !in_apply_phase_) {
		// Loud-but-no-crash (same policy as immutable_or_log_ below): the structural op
		// is refused — the caller early-returns its failure value — and an error is
		// logged so the misuse doesn't hide behind a surprise nullptr/false/invalid-id
		// return.
		spdlog::error_s(
			"{} refused: direct structural mutation during tick_systems — queue the op on "
			"ctx.cmd instead (applies at the stage barrier)",
			fn_name
		);
		return true;
	}
	return false;
}

bool World::immutable_or_log_(EntityID id, char const* fn_name, std::string_view component_name) const {
	// Caller has already passed is_alive(id), so id.index is in range and the slot is finalised.
	if (entity_slots[id.index].immutable) {
		// Loud-but-no-crash: the structural op is refused (the caller early-returns) and we log
		// an error. This is the runtime backstop for plain EntityIDs that surface for an immutable
		// entity (for_each_with_entity, or a laundered unsafe_mutable_id()).
		spdlog::error_s(
			"{} refused: entity {}:{} is immutable — its archetype cannot change (component '{}')",
			fn_name, id.index, id.generation, component_name
		);
		return true;
	}
	return false;
}

bool World::is_immutable(EntityID id) const {
	if (!is_alive(id)) {
		return false;
	}
	return entity_slots[id.index].immutable;
}

World::~World() {
	// Destroy each system instance via its type-erased deleter.
	for (SystemRegistration& reg : system_registry_) {
		if (reg.alive && reg.instance != nullptr && reg.deleter != nullptr) {
			reg.deleter(reg.instance);
			reg.instance = nullptr;
		}
	}
	// Explicit pool drain: destroys live components and returns every chunk's block to the
	// pool. After this loop runs, each Archetype's `chunks` vector is empty, so the
	// (non-pool fallback) Archetype destructor that fires during `archetypes` vector teardown
	// has nothing left to do. `chunk_pool_` itself destroys after `archetypes` (declaration
	// order — see World.hpp) and frees its cached blocks.
	for (Archetype& arch : archetypes) {
		arch.drain_to_pool(chunk_pool_);
	}
	// pending_cmds_, scheduler_, ecs_thread_pool_ are unique_ptr — clean up automatically.
}

EntityID World::allocate_entity_slot() {
	uint32_t index;
	if (has_free) {
		index = first_free;
		EntitySlot& slot = entity_slots[index];
		has_free = (slot.next_free != index);
		first_free = slot.next_free;
		slot.alive = true;
		slot.archetype_index = 0;
		slot.chunk_index = 0;
		slot.row = 0;
		// Single authoritative reset: a reused slot must never inherit a prior occupant's
		// immutability. create_immutable_entity re-stamps this true after the slot is filled.
		slot.immutable = false;
		// Generation incremented on each reuse so old EntityIDs become invalid. Skip both
		// 0 (the INVALID sentinel) and any value with DEFERRED_GENERATION_BIT set — the
		// high bit is reserved for CommandBuffer placeholder EntityIDs, so real generations
		// stay in [1, 0x7FFFFFFF].
		slot.generation += 1;
		if (slot.generation == 0 || (slot.generation & DEFERRED_GENERATION_BIT) != 0) {
			slot.generation = 1;
		}
	} else {
		index = static_cast<uint32_t>(entity_slots.size());
		EntitySlot fresh;
		fresh.generation = 1;
		fresh.alive = true;
		entity_slots.push_back(fresh);
	}
	return EntityID { index, entity_slots[index].generation };
}

EntityID World::reserve_entity_slot() {
	EntityID const eid = allocate_entity_slot();
	// Mark as reserved-but-unfinalised: alive == true (slot is held), but archetype_index
	// is the sentinel. is_alive() filters this out — reserved slots are observable only
	// through the EntityID returned to the reserver.
	entity_slots[eid.index].archetype_index = INVALID_ARCHETYPE;
	entity_slots[eid.index].chunk_index = 0;
	entity_slots[eid.index].row = 0;
	return eid;
}

void World::compute_chunk_layout(Archetype& arch) {
	arch.column_offsets.assign(arch.signature.size(), NO_COLUMN_OFFSET);

	// Pessimistic capacity: every non-tag column might need up to (align - 1) slack to
	// align its slab start. Account for that worst-case overhead so the lay-out below is
	// guaranteed to fit.
	std::size_t total_per_row = sizeof(EntityID);
	std::size_t total_align_overhead = 0;
	for (std::size_t i = 0; i < arch.signature.size(); ++i) {
		ColumnVTable const* vt = arch.vtables[i];
		if (vt->size == 0) {
			continue;
		}
		total_per_row += vt->size;
		if (vt->align > 0) {
			total_align_overhead += (vt->align - 1);
		}
	}
	std::size_t const usable = (CHUNK_BLOCK_BYTES > total_align_overhead)
		? (CHUNK_BLOCK_BYTES - total_align_overhead)
		: CHUNK_BLOCK_BYTES;
	std::size_t capacity = std::max<std::size_t>(1, usable / total_per_row);

	// Compute slab offsets at this capacity. If the result happens to overflow (only
	// possible at degenerate edge cases — gigantic alignments, etc.), back off.
	auto layout_at_capacity = [&](std::size_t cap) -> std::size_t {
		arch.column_offsets.assign(arch.signature.size(), NO_COLUMN_OFFSET);
		std::size_t offset = cap * sizeof(EntityID);
		for (std::size_t i = 0; i < arch.signature.size(); ++i) {
			ColumnVTable const* vt = arch.vtables[i];
			if (vt->size == 0) {
				continue;
			}
			if (vt->align > 1) {
				offset = (offset + vt->align - 1) & ~(vt->align - 1);
			}
			arch.column_offsets[i] = offset;
			offset += cap * vt->size;
		}
		return offset;
	};

	std::size_t total = layout_at_capacity(capacity);
	while (total > CHUNK_BLOCK_BYTES && capacity > 1) {
		--capacity;
		total = layout_at_capacity(capacity);
	}

	arch.chunk_capacity = capacity;
}

uint32_t World::find_or_create_archetype(
	std::vector<component_type_id_t> const& sig, ColumnVTable const* const* vtables
) {
	auto it = archetype_by_signature.find(sig);
	if (it != archetype_by_signature.end()) {
		return it->second;
	}

	uint32_t const idx = static_cast<uint32_t>(archetypes.size());
	Archetype arch;
	arch.signature = sig;
	arch.vtables.assign(vtables, vtables + sig.size());
	arch.column_versions.assign(sig.size(), 0);
	compute_chunk_layout(arch);
	arch.matcher_hash = compute_matcher_hash(sig);
	arch.chunk_pool = &chunk_pool_;
	// Chunks vector is empty until first row is reserved — `reserve_row` allocates lazily.
	archetypes.push_back(std::move(arch));
	archetype_by_signature.emplace(sig, idx);
	// New archetype — bump epoch so cached query results that don't include this index
	// will be rebuilt on next access.
	archetype_epoch += 1;
	if (archetype_epoch == 0) {
		// Wraparound (would take 4 billion archetype creations). Force a cache flush so a
		// zero `cached.epoch` doesn't accidentally compare equal to the wrapped value.
		query_cache.clear();
		archetype_epoch = 1;
	}
	return idx;
}

bool World::is_alive(EntityID id) const {
	// Deferred-create placeholder returned by CommandBuffer::create_entity in parallel mode —
	// not a real EntityID; usable only as an argument to other ops on the same buffer until
	// CommandBuffer::apply resolves it. Returning false here makes a stray placeholder safe
	// to pass to any World accessor (every templated accessor flows through is_alive).
	if (id.is_deferred()) {
		return false;
	}
	if (id.index >= entity_slots.size()) {
		return false;
	}
	EntitySlot const& slot = entity_slots[id.index];
	if (!slot.alive || slot.generation != id.generation) {
		return false;
	}
	// Reserved-but-unfinalised slot: addressable by ID but not yet "alive" to the rest of
	// the API. `is_alive` returns false for these — they have no archetype/components.
	if (slot.archetype_index == INVALID_ARCHETYPE) {
		return false;
	}
	return true;
}

void World::compact_archetype_after_external_move(
	uint32_t archetype_index, std::size_t chunk_index, std::size_t row
) {
	Archetype& arch = archetypes[archetype_index];
	for (uint64_t& v : arch.column_versions) {
		++v;
	}

	std::size_t const last_chunk = arch.chunks.size() - 1;
	std::size_t const last_row = arch.chunks[last_chunk].count - 1;

	bool const same_slot = (chunk_index == last_chunk && row == last_row);
	if (!same_slot) {
		// Move every column's data from (last_chunk, last_row) to (chunk_index, row).
		for (std::size_t i = 0; i < arch.signature.size(); ++i) {
			if (arch.column_offsets[i] == NO_COLUMN_OFFSET) {
				continue;
			}
			void* dst = arch.row_in_column(chunk_index, i, row);
			void* src = arch.row_in_column(last_chunk, i, last_row);
			arch.vtables[i]->move_construct(dst, src);
		}
		// Update relocated entity's slot to point at the new (chunk, row).
		EntityID const moved = arch.entity_array(last_chunk)[last_row];
		entity_slots[moved.index].chunk_index = static_cast<uint32_t>(chunk_index);
		entity_slots[moved.index].row = static_cast<uint32_t>(row);
		arch.entity_array(chunk_index)[row] = moved;
	}

	// Drop the trailing row from the last chunk.
	--arch.chunks[last_chunk].count;
	--arch.total_entity_count;

	// If the trailing chunk is now empty, return its block to the pool. No retain-one
	// rule — a fully-drained archetype shrinks chunks to size 0, and the next reserve_row
	// pulls a warm block back from the pool at the same cost as indexing into a kept slot.
	if (arch.chunks.back().count == 0) {
		DataChunk& back = arch.chunks.back();
		chunk_pool_.release(back.data);
		back.data = nullptr;
		arch.chunks.pop_back();
	}
}

void World::destroy_entity(EntityID id) {
	if (in_tick_or_log_("World::destroy_entity")) {
		return;
	}
	// Deferred placeholder — never references a real slot. The CommandBuffer's apply() resolves
	// placeholders to real EIDs before calling destroy_entity, so this guard only fires for
	// stray placeholders leaked outside their buffer.
	if (id.is_deferred()) {
		return;
	}
	if (id.index >= entity_slots.size()) {
		return;
	}
	EntitySlot& slot = entity_slots[id.index];
	if (!slot.alive || slot.generation != id.generation) {
		return;
	}

	// Reserved-but-unfinalised slot: just drop the reservation, no archetype work.
	if (slot.archetype_index == INVALID_ARCHETYPE) {
		drop_reserved_slot(id);
		return;
	}

	uint32_t const archetype_index = slot.archetype_index;
	std::size_t const removed_chunk = slot.chunk_index;
	std::size_t const removed_row = slot.row;

	// Destroy each non-tag component in place, then compact via the shared external-move helper.
	{
		Archetype& arch = archetypes[archetype_index];
		for (std::size_t i = 0; i < arch.signature.size(); ++i) {
			if (arch.column_offsets[i] == NO_COLUMN_OFFSET) {
				continue;
			}
			arch.vtables[i]->destroy(arch.row_in_column(removed_chunk, i, removed_row));
		}
	}
	compact_archetype_after_external_move(archetype_index, removed_chunk, removed_row);

	slot.alive = false;
	slot.next_free = has_free ? first_free : id.index;
	first_free = id.index;
	has_free = true;
}

void World::drop_reserved_slot(EntityID id) {
	if (id.index >= entity_slots.size()) {
		return;
	}
	EntitySlot& slot = entity_slots[id.index];
	if (!slot.alive || slot.generation != id.generation) {
		return;
	}
	if (slot.archetype_index != INVALID_ARCHETYPE) {
		return; // already finalised
	}
	slot.alive = false;
	slot.next_free = has_free ? first_free : id.index;
	first_free = id.index;
	has_free = true;
}

bool World::snapshot_identity(WorldIdentitySnapshot& out) const {
	out.slots.clear();
	out.free_list.clear();

	if (in_tick_) {
		spdlog::error_s("World::snapshot_identity refused: called mid-tick — snapshot only between ticks");
		return false;
	}

	// Single pass: record every slot's current generation (+ immutable for live slots), refuse
	// reserved-but-unfinalised slots, and count the dead slots the free chain must account for.
	std::size_t dead_count = 0;
	out.slots.reserve(entity_slots.size());
	for (std::size_t i = 0; i < entity_slots.size(); ++i) {
		EntitySlot const& slot = entity_slots[i];
		if (slot.alive && slot.archetype_index == INVALID_ARCHETYPE) {
			spdlog::error_s(
				"World::snapshot_identity refused: slot {} is reserved-but-unfinalised (a CommandBuffer "
				"holds un-applied creates) — apply every buffer before snapshotting",
				i
			);
			out.slots.clear();
			return false;
		}
		WorldIdentitySnapshot::SlotRecord rec;
		rec.generation = slot.generation;
		// Normalize: a dead slot's immutable bit is unobservable leftover state (reset on reuse).
		rec.immutable = slot.alive ? slot.immutable : false;
		out.slots.push_back(rec);
		if (!slot.alive) {
			++dead_count;
		}
	}

	// Walk the free chain in pop order, guarding against corruption: every node must be an
	// in-range dead slot, and the walk must terminate (self-pointing tail) within slot count.
	if (has_free) {
		uint32_t cur = first_free;
		std::size_t steps = 0;
		while (true) {
			if (cur >= entity_slots.size() || entity_slots[cur].alive || steps >= entity_slots.size()) {
				spdlog::error_s(
					"World::snapshot_identity refused: corrupt free chain at slot {} (step {})", cur, steps
				);
				out.slots.clear();
				out.free_list.clear();
				return false;
			}
			out.free_list.push_back(cur);
			++steps;
			uint32_t const next = entity_slots[cur].next_free;
			if (next == cur) {
				break; // self-pointing tail sentinel
			}
			cur = next;
		}
	}

	// Every dead slot must be chain-reachable — a mismatch means a leaked or doubly-linked slot.
	if (out.free_list.size() != dead_count) {
		spdlog::error_s(
			"World::snapshot_identity refused: free chain holds {} slots but {} slots are dead",
			out.free_list.size(), dead_count
		);
		out.slots.clear();
		out.free_list.clear();
		return false;
	}

	return true;
}

bool World::restore_identity(WorldIdentitySnapshot const& snapshot) {
	if (in_tick_) {
		spdlog::error_s("World::restore_identity refused: called mid-tick");
		return false;
	}
	if (!entity_slots.empty()) {
		spdlog::error_s(
			"World::restore_identity refused: target World is not fresh ({} slots already allocated)",
			entity_slots.size()
		);
		return false;
	}

	// Validate fully before mutating — on any failure the World stays untouched.
	for (std::size_t i = 0; i < snapshot.slots.size(); ++i) {
		uint32_t const gen = snapshot.slots[i].generation;
		if (gen == 0 || (gen & DEFERRED_GENERATION_BIT) != 0) {
			spdlog::error_s(
				"World::restore_identity refused: slot {} has invalid generation {:#x} (must be in [1, 0x7FFFFFFF])",
				i, gen
			);
			return false;
		}
	}
	{
		std::vector<bool> seen(snapshot.slots.size(), false);
		for (uint32_t const free_index : snapshot.free_list) {
			if (free_index >= snapshot.slots.size()) {
				spdlog::error_s(
					"World::restore_identity refused: free-list index {} out of range ({} slots)",
					free_index, snapshot.slots.size()
				);
				return false;
			}
			if (seen[free_index]) {
				spdlog::error_s("World::restore_identity refused: duplicate free-list index {}", free_index);
				return false;
			}
			seen[free_index] = true;
		}
	}

	// Rebuild the slot table: every slot starts as a live reserved-but-unfinalised slot (the
	// loader finalises each via restore_entity), then the free list overwrites its members.
	entity_slots.resize(snapshot.slots.size());
	for (std::size_t i = 0; i < snapshot.slots.size(); ++i) {
		EntitySlot& slot = entity_slots[i];
		slot.archetype_index = INVALID_ARCHETYPE;
		slot.chunk_index = 0;
		slot.row = 0;
		slot.generation = snapshot.slots[i].generation;
		slot.next_free = 0;
		slot.alive = true;
		slot.immutable = snapshot.slots[i].immutable;
	}
	for (std::size_t j = 0; j < snapshot.free_list.size(); ++j) {
		uint32_t const index = snapshot.free_list[j];
		EntitySlot& slot = entity_slots[index];
		slot.alive = false;
		slot.immutable = false;
		slot.archetype_index = 0;
		// Chain in pop order; the tail self-points (the sentinel allocate_entity_slot expects).
		slot.next_free = (j + 1 < snapshot.free_list.size()) ? snapshot.free_list[j + 1] : index;
	}
	has_free = !snapshot.free_list.empty();
	first_free = has_free ? snapshot.free_list.front() : 0;

	return true;
}

bool World::restore_entity_precheck_(EntityID eid) const {
	if (eid.is_deferred()) {
		spdlog::error_s("World::restore_entity refused: {}:{:#x} is a deferred placeholder", eid.index, eid.generation);
		return false;
	}
	if (eid.index >= entity_slots.size()) {
		spdlog::error_s(
			"World::restore_entity refused: slot index {} out of range ({} slots)", eid.index, entity_slots.size()
		);
		return false;
	}
	EntitySlot const& slot = entity_slots[eid.index];
	if (!slot.alive) {
		spdlog::error_s("World::restore_entity refused: slot {} is dead (free at snapshot time)", eid.index);
		return false;
	}
	if (slot.generation != eid.generation) {
		spdlog::error_s(
			"World::restore_entity refused: stale generation for entity {}:{} (slot holds {})",
			eid.index, eid.generation, slot.generation
		);
		return false;
	}
	if (slot.archetype_index != INVALID_ARCHETYPE) {
		spdlog::error_s(
			"World::restore_entity refused: entity {}:{} is already finalised", eid.index, eid.generation
		);
		return false;
	}
	return true;
}

void World::finalize_reserved_entity(
	EntityID eid,
	std::vector<component_type_id_t> const& sorted_sig,
	std::vector<ColumnVTable const*> const& sorted_vtables,
	std::vector<void*> const& sorted_value_slots
) {
	if (eid.index >= entity_slots.size()) {
		return;
	}
	EntitySlot& slot = entity_slots[eid.index];
	if (!slot.alive || slot.generation != eid.generation) {
		return;
	}
	if (slot.archetype_index != INVALID_ARCHETYPE) {
		return; // already finalised
	}

	uint32_t const arch_idx = find_or_create_archetype(sorted_sig, sorted_vtables.data());
	Archetype::RowLocation loc = archetypes[arch_idx].reserve_row();
	archetypes[arch_idx].entity_array(loc.chunk_index)[loc.row] = eid;

	for (std::size_t i = 0; i < sorted_sig.size(); ++i) {
		if (sorted_vtables[i]->size == 0) {
			continue; // tag column
		}
		void* dst = archetypes[arch_idx].row_in_column(loc.chunk_index, i, loc.row);
		// Move-construct from the caller-provided slot into the archetype slab.
		sorted_vtables[i]->move_construct(dst, sorted_value_slots[i]);
	}

	slot.archetype_index = arch_idx;
	slot.chunk_index = static_cast<uint32_t>(loc.chunk_index);
	slot.row = static_cast<uint32_t>(loc.row);
}

bool World::bulk_create_sizes_ok_(
	std::size_t count, std::size_t out_ids_size, std::size_t const* span_sizes,
	std::size_t span_count, char const* fn_name
) const {
	if (out_ids_size != count) {
		spdlog::error_s("{} refused: out_ids length {} != count {}", fn_name, out_ids_size, count);
		return false;
	}
	for (std::size_t i = 0; i < span_count; ++i) {
		if (span_sizes[i] != count) {
			spdlog::error_s(
				"{} refused: input span {} has length {} != count {}", fn_name, i, span_sizes[i], count
			);
			return false;
		}
	}
	return true;
}

void World::finalize_reserved_entities_bulk(
	std::span<EntityID const> ids,
	std::vector<component_type_id_t> const& sorted_sig,
	std::vector<ColumnVTable const*> const& sorted_vtables,
	std::span<void* const> column_blocks
) {
	std::size_t const count = ids.size();
	if (count == 0) {
		return;
	}

	// Pre-validate every id with the same four checks finalize_reserved_entity applies.
	bool all_valid = true;
	for (EntityID const eid : ids) {
		if (eid.index >= entity_slots.size()) {
			all_valid = false;
			break;
		}
		EntitySlot const& slot = entity_slots[eid.index];
		if (!slot.alive || slot.generation != eid.generation || slot.archetype_index != INVALID_ARCHETYPE) {
			all_valid = false;
			break;
		}
	}

	if (!all_valid) {
		// Cold fallback: per-entity finalize over per-element block pointers, preserving
		// loop-of-single-creates semantics — only the bad slots are skipped. Skipped
		// entities' staged values are destroyed here; finalised entities' values are
		// destroyed by the move-construct. Either way the caller frees the raw blocks
		// afterwards without touching elements again.
		std::vector<void*> element_slots(sorted_sig.size(), nullptr);
		for (std::size_t i = 0; i < count; ++i) {
			EntityID const eid = ids[i];
			for (std::size_t c = 0; c < sorted_sig.size(); ++c) {
				if (sorted_vtables[c]->size == 0 || column_blocks[c] == nullptr) {
					element_slots[c] = nullptr;
					continue;
				}
				element_slots[c]
					= static_cast<unsigned char*>(column_blocks[c]) + i * sorted_vtables[c]->size;
			}
			bool const valid = eid.index < entity_slots.size()
				&& entity_slots[eid.index].alive
				&& entity_slots[eid.index].generation == eid.generation
				&& entity_slots[eid.index].archetype_index == INVALID_ARCHETYPE;
			if (valid) {
				finalize_reserved_entity(eid, sorted_sig, sorted_vtables, element_slots);
			} else {
				for (std::size_t c = 0; c < sorted_sig.size(); ++c) {
					if (element_slots[c] != nullptr) {
						sorted_vtables[c]->destroy(element_slots[c]);
					}
				}
			}
		}
		return;
	}

	// Fast path: one archetype lookup, bulk row reservation, column-contiguous moves.
	uint32_t const arch_idx = find_or_create_archetype(sorted_sig, sorted_vtables.data());
	Archetype& arch = archetypes[arch_idx];
	arch.reserve_rows(count, bulk_rows_scratch_);

	{
		std::size_t i = 0;
		for (Archetype::RowRange const& range : bulk_rows_scratch_) {
			EntityID* const earr = arch.entity_array(range.chunk_index);
			for (std::size_t k = 0; k < range.count; ++k, ++i) {
				EntityID const eid = ids[i];
				earr[range.row_begin + k] = eid;
				EntitySlot& slot = entity_slots[eid.index];
				slot.archetype_index = arch_idx;
				slot.chunk_index = static_cast<uint32_t>(range.chunk_index);
				slot.row = static_cast<uint32_t>(range.row_begin + k);
			}
		}
	}

	for (std::size_t c = 0; c < sorted_sig.size(); ++c) {
		if (sorted_vtables[c]->size == 0 || column_blocks[c] == nullptr) {
			continue;
		}
		std::size_t offset = 0;
		for (Archetype::RowRange const& range : bulk_rows_scratch_) {
			void* dst = arch.row_in_column(range.chunk_index, c, range.row_begin);
			void* src = static_cast<unsigned char*>(column_blocks[c]) + offset * sorted_vtables[c]->size;
			sorted_vtables[c]->move_construct_n(dst, src, range.count);
			offset += range.count;
		}
	}
}

World::CachedQuery const& World::resolve_query_cache(QueryCacheKey const& key) const {
	auto it = query_cache.find(key);
	if (it != query_cache.end() && it->second.epoch == archetype_epoch) {
		return it->second;
	}

	CachedQuery rebuilt;
	rebuilt.epoch = archetype_epoch;
	for (component_type_id_t id : key.require_ids) {
		rebuilt.require_matcher |= (uint64_t { 1 } << (id % 63));
	}
	for (component_type_id_t id : key.exclude_ids) {
		rebuilt.exclude_matcher |= (uint64_t { 1 } << (id % 63));
	}
	for (std::size_t i = 0; i < archetypes.size(); ++i) {
		Archetype const& arch = archetypes[i];
		// Bitfield prefilter: O(1) reject before the sorted-set walk.
		if ((arch.matcher_hash & rebuilt.require_matcher) != rebuilt.require_matcher) {
			continue;
		}
		if ((arch.matcher_hash & rebuilt.exclude_matcher) != 0) {
			// Possible exclude overlap. Sorted-set walk below will confirm; bitfield collisions
			// (id % 63) mean this isn't authoritative.
			if (!arch.matches_none(key.exclude_ids)) {
				continue;
			}
		}
		if (!arch.matches_all(key.require_ids)) {
			continue;
		}
		rebuilt.archetype_indices.push_back(static_cast<uint32_t>(i));
	}

	if (it == query_cache.end()) {
		auto inserted = query_cache.emplace(key, std::move(rebuilt));
		return inserted.first->second;
	}
	it->second = std::move(rebuilt);
	return it->second;
}

CommandBuffer* World::allocate_pending_cmd_() {
	// unique_ptr keeps the CB heap-stable across pending_cmds_ growth. Defined here (not in the
	// register_system template) so the incomplete forward-declared CommandBuffer never reaches
	// a make_unique / vector<unique_ptr<CommandBuffer>> destroy path in a translation unit that
	// lacks CommandBuffer.hpp — CommandBuffer is complete in this TU.
	pending_cmds_.push_back(std::make_unique<CommandBuffer>());
	return pending_cmds_.back().get();
}

void World::unregister_system(SystemHandle handle) {
	if (!handle.is_valid() || handle.index >= system_registry_.size()) {
		return;
	}
	SystemRegistration& reg = system_registry_[handle.index];
	if (!reg.alive || reg.generation != handle.generation) {
		return;
	}
	reg.alive = false;
	if (reg.instance != nullptr && reg.deleter != nullptr) {
		reg.deleter(reg.instance);
	}
	reg.instance = nullptr;
	reg.tick_all_fn = nullptr;
	reg.generation += 1;
	if (reg.generation == 0) {
		reg.generation = 1;
	}
	scheduler_dirty_ = true;
}

void World::tick_systems(TickContext const& /*ctx_in*/) {
	// Note: the legacy signature of tick_systems took an externally constructed
	// TickContext that referenced its own CommandBuffer. The new model gives every
	// system its own pending_cmd, so we ignore the caller-supplied buffer and
	// construct per-system contexts inside SystemScheduler::run. We keep this
	// overload for source compatibility — callers should use `tick_systems(today)`.
	tick_systems(/*ctx_in.today*/ Date {});
}

void World::tick_systems(Date today) {
	if (!scheduler_) {
		scheduler_ = std::make_unique<SystemScheduler>();
		scheduler_dirty_ = true;
	}
	if (scheduler_dirty_) {
		scheduler_->rebuild(system_registry_);
		scheduler_dirty_ = false;
	}
	in_tick_ = true;
	scheduler_->run(*this, today, system_registry_, ecs_thread_pool(), serial_mode_);
	in_tick_ = false;
	current_system_registration_ = nullptr;
	// Advance the chunk pool's aging clock — frees blocks whose release tick is older
	// than ChunkPool::AGE_THRESHOLD_TICKS. Working sets that keep cycling refresh their
	// release tick each iteration and stay warm.
	chunk_pool_.advance_tick();
}

void World::clear_systems() {
	for (SystemRegistration& reg : system_registry_) {
		if (reg.alive && reg.instance != nullptr && reg.deleter != nullptr) {
			reg.deleter(reg.instance);
		}
	}
	system_registry_.clear();
	pending_cmds_.clear();
	scheduler_dirty_ = true;
	if (scheduler_) {
		scheduler_->invalidate();
	}
}

uint64_t World::schedule_hash() {
	if (!scheduler_) {
		scheduler_ = std::make_unique<SystemScheduler>();
		scheduler_dirty_ = true;
	}
	if (scheduler_dirty_) {
		scheduler_->rebuild(system_registry_);
		scheduler_dirty_ = false;
	}
	return scheduler_->schedule_hash();
}

std::size_t World::debug_stage_count() {
	if (!scheduler_) {
		scheduler_ = std::make_unique<SystemScheduler>();
		scheduler_dirty_ = true;
	}
	if (scheduler_dirty_) {
		scheduler_->rebuild(system_registry_);
		scheduler_dirty_ = false;
	}
	return scheduler_->stage_count();
}

std::size_t World::debug_stage_index_of(system_type_id_t type_id) {
	if (!scheduler_) {
		scheduler_ = std::make_unique<SystemScheduler>();
		scheduler_dirty_ = true;
	}
	if (scheduler_dirty_) {
		scheduler_->rebuild(system_registry_);
		scheduler_dirty_ = false;
	}
	return scheduler_->stage_index_of(type_id, system_registry_);
}

void World::set_serial_mode(bool enabled) {
	serial_mode_ = enabled;
}

void World::set_ecs_worker_count(uint32_t count) {
	ecs_worker_count_ = count;
	// If pool already exists, rebuild it next access.
	ecs_thread_pool_.reset();
}

EcsThreadPool& World::ecs_thread_pool() {
	if (!ecs_thread_pool_) {
		uint32_t n = ecs_worker_count_;
		if (n == 0) {
			uint32_t hw = static_cast<uint32_t>(std::thread::hardware_concurrency());
			if (hw == 0) {
				hw = 1;
			}
			n = std::min<uint32_t>(hw, 16u);
		}
		ecs_thread_pool_ = std::make_unique<EcsThreadPool>(n);
	}
	return *ecs_thread_pool_;
}

SystemRegistration* World::current_system_registration() {
	return current_system_registration_;
}

std::size_t World::archetype_chunk_count(uint32_t archetype_idx) const {
	return archetypes[archetype_idx].chunks.size();
}

std::size_t World::archetype_chunk_row_count(uint32_t archetype_idx, std::size_t chunk_idx) const {
	return archetypes[archetype_idx].chunks[chunk_idx].count;
}

World::CachedQuery const& World::resolve_query_cache_for_threaded(QueryCacheKey const& key) const {
	return resolve_query_cache(key);
}
