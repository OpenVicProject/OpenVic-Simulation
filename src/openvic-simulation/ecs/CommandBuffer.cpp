#include "openvic-simulation/ecs/CommandBuffer.hpp"

#include <cstddef>
#include <vector>

#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;

void CommandBuffer::apply(World& world) {
	// Resolution map for deferred placeholders. Indexed by placeholder local_seq (= op.eid.index
	// for any deferred eid). Empty when the buffer holds no deferred ops — the common case for
	// serial-system buffers, where this allocation is a no-op.
	std::vector<EntityID> placeholder_to_real;
	if (deferred_count_ > 0) {
		placeholder_to_real.assign(deferred_count_, INVALID_ENTITY_ID);
	}
	auto resolve = [&](EntityID eid) -> EntityID {
		if (!eid.is_deferred()) {
			return eid;
		}
		if (eid.index >= placeholder_to_real.size()) {
			return INVALID_ENTITY_ID; // out-of-range placeholder — shouldn't happen
		}
		return placeholder_to_real[eid.index];
	};

	for (Op& op : ops) {
		switch (op.kind) {
			case OpKind::CreateEntity: {
				// In parallel-mode-recorded ops, op.eid is a deferred placeholder. Allocate a
				// real slot here (single-threaded, deterministic order) and store the mapping
				// before we finalise — subsequent ops that reference this placeholder resolve
				// via the map. Serial-mode ops already carry a real reserved EntityID and skip
				// the allocation step.
				EntityID real_eid = op.eid;
				bool const was_deferred = op.eid.is_deferred();
				if (was_deferred) {
					real_eid = world.reserve_entity_slot();
					if (op.eid.index < placeholder_to_real.size()) {
						placeholder_to_real[op.eid.index] = real_eid;
					}
				}
				// Hand the World move-only payload pointers; finalize_reserved_entity moves
				// them into the archetype's column slabs. After the call, the payload slots
				// are moved-from but still need their allocations freed.
				std::vector<void*> raw_slots;
				raw_slots.reserve(op.create.sorted_values.size());
				for (PayloadSlot& slot : op.create.sorted_values) {
					raw_slots.push_back(slot.data);
				}
				world.finalize_reserved_entity(
					real_eid, op.create.sorted_sig, op.create.sorted_vtables, raw_slots
				);
				// Stamp immutability onto the freshly-finalised slot (covers both the deferred
				// and serial create_immutable_entity paths in one place). CommandBuffer is a
				// friend of World, so entity_slots is reachable here.
				if (op.create.immutable && real_eid.index < world.entity_slots.size()) {
					world.entity_slots[real_eid.index].immutable = true;
				}
				// Free the moved-from payload allocations. We don't call destroy() on them —
				// the move-construct already destructively transferred the value. Capture
				// `align` into a local BEFORE `release_data()` — `release_data()` clears the
				// slot's `vtable` pointer, and the argument-evaluation order for the delete
				// call below is unspecified, so any access to `slot.vtable->align` in the
				// same expression is undefined.
				for (PayloadSlot& slot : op.create.sorted_values) {
					if (slot.data != nullptr && slot.vtable != nullptr && slot.vtable->size > 0) {
						std::size_t const align = slot.vtable->align;
						::operator delete(slot.release_data(), std::align_val_t { align });
						slot.vtable = nullptr;
					}
				}
				break;
			}
			case OpKind::DestroyEntity: {
				// World::destroy_entity is a no-op on dead entities and on
				// reserved-but-unfinalised slots (calls drop_reserved_slot internally).
				world.destroy_entity(resolve(op.eid));
				break;
			}
			case OpKind::AddComponent: {
				// Build a single-component sorted signature against the entity's current
				// archetype + the new id, then dispatch through the existing template
				// add_component path is awkward (requires the type at the call site). We
				// instead replicate the migration logic at the type-erased level: find or
				// create the target archetype and move the new component plus all existing
				// components over.
				//
				// For simplicity this implementation goes through add_component_typeerased
				// (a private World helper added below). If the entity already carries C,
				// the existing slot is overwritten via move-assign… but move-assign isn't
				// available type-erased. So if the component already exists, we destroy
				// the existing value first then move-construct the new one in place.
				EntityID const eid = resolve(op.eid);
				if (!eid.is_valid() || eid.is_deferred()) {
					// Unresolved placeholder (a same-buffer add referencing a placeholder whose
					// CreateEntity op never ran, e.g. due to allocation failure). Drop silently.
					break;
				}
				if (eid.index >= world.entity_slots.size()) {
					break;
				}
				EntitySlot const& slot = world.entity_slots[eid.index];
				if (!slot.alive || slot.generation != eid.generation) {
					break;
				}
				if (slot.archetype_index == INVALID_ARCHETYPE) {
					// Entity is reserved-but-unfinalised — adding a component before the
					// CreateEntity op has been applied is undefined-by-policy. Ignore.
					break;
				}
				if (slot.immutable) {
						break;
					}
					// Immutability backstop (deferred path): apply() migrates type-erased here,
					// NOT through World::add_component, so the refusal is repeated. The intact
					// op.add.value payload is freed by its PayloadSlot destructor at ops.clear().
					ColumnVTable const* new_vt = op.add.value.vtable;
				component_type_id_t const new_id = op.add.id;
				uint32_t const src_idx = slot.archetype_index;
				uint32_t const src_chunk = slot.chunk_index;
				uint32_t const src_row = slot.row;

				// In-place replace if already present.
				{
					Archetype& src = world.archetypes[src_idx];
					std::size_t const existing_col = src.column_index_for(new_id);
					if (existing_col != NO_COLUMN_INDEX) {
						if (src.column_offsets[existing_col] != NO_COLUMN_OFFSET) {
							void* dst = src.row_in_column(src_chunk, existing_col, src_row);
							src.vtables[existing_col]->destroy(dst);
							src.vtables[existing_col]->move_construct(dst, op.add.value.data);
							++src.column_versions[existing_col];
						}
						// Free the moved-from payload allocation. Capture `align` BEFORE
						// `release_data()` clears the vtable pointer — see CreateEntity branch
						// for the order-of-evaluation rationale.
						if (op.add.value.data != nullptr && op.add.value.vtable != nullptr
							&& op.add.value.vtable->size > 0) {
							std::size_t const align = op.add.value.vtable->align;
							::operator delete(
								op.add.value.release_data(), std::align_val_t { align }
							);
							op.add.value.vtable = nullptr;
						}
						break;
					}
				}

				// Build target signature = src.signature ∪ {new_id}, sorted ascending.
				std::vector<component_type_id_t> target_sig;
				std::vector<ColumnVTable const*> target_vtables;
				{
					Archetype const& src = world.archetypes[src_idx];
					target_sig.reserve(src.signature.size() + 1);
					target_vtables.reserve(src.signature.size() + 1);
					bool inserted = false;
					for (std::size_t i = 0; i < src.signature.size(); ++i) {
						component_type_id_t const sid = src.signature[i];
						if (!inserted && sid > new_id) {
							target_sig.push_back(new_id);
							target_vtables.push_back(new_vt);
							inserted = true;
						}
						target_sig.push_back(sid);
						target_vtables.push_back(src.vtables[i]);
					}
					if (!inserted) {
						target_sig.push_back(new_id);
						target_vtables.push_back(new_vt);
					}
				}

				uint32_t const target_idx
					= world.find_or_create_archetype(target_sig, target_vtables.data());

				Archetype::RowLocation target_loc = world.archetypes[target_idx].reserve_row();
				world.archetypes[target_idx].entity_array(target_loc.chunk_index)[target_loc.row] = eid;

				{
					Archetype& target = world.archetypes[target_idx];
					Archetype& src = world.archetypes[src_idx];
					for (std::size_t i = 0; i < target.signature.size(); ++i) {
						component_type_id_t const tid = target.signature[i];
						if (target.column_offsets[i] == NO_COLUMN_OFFSET) {
							continue; // tag column — no data
						}
						void* dst = target.row_in_column(
							target_loc.chunk_index, i, target_loc.row
						);
						if (tid == new_id) {
							target.vtables[i]->move_construct(dst, op.add.value.data);
						} else {
							std::size_t const src_col_idx = src.column_index_for(tid);
							void* srcp = src.row_in_column(src_chunk, src_col_idx, src_row);
							target.vtables[i]->move_construct(dst, srcp);
						}
					}
				}

				world.compact_archetype_after_external_move(src_idx, src_chunk, src_row);

				EntitySlot& mutable_slot = world.entity_slots[eid.index];
				mutable_slot.archetype_index = target_idx;
				mutable_slot.chunk_index = static_cast<uint32_t>(target_loc.chunk_index);
				mutable_slot.row = static_cast<uint32_t>(target_loc.row);

				// Free the moved-from payload allocation. Capture `align` BEFORE
				// `release_data()` clears the vtable pointer — see CreateEntity branch
				// for the order-of-evaluation rationale.
				if (op.add.value.data != nullptr && op.add.value.vtable != nullptr
					&& op.add.value.vtable->size > 0) {
					std::size_t const align = op.add.value.vtable->align;
					::operator delete(
						op.add.value.release_data(), std::align_val_t { align }
					);
					op.add.value.vtable = nullptr;
				}
				break;
			}
			case OpKind::RemoveComponent: {
				EntityID const eid = resolve(op.eid);
				if (!eid.is_valid() || eid.is_deferred()) {
					break;
				}
				if (eid.index >= world.entity_slots.size()) {
					break;
				}
				EntitySlot const& slot = world.entity_slots[eid.index];
				if (!slot.alive || slot.generation != eid.generation) {
					break;
				}
				if (slot.archetype_index == INVALID_ARCHETYPE) {
					break;
				}
				uint32_t const src_idx = slot.archetype_index;
				uint32_t const src_chunk = slot.chunk_index;
				uint32_t const src_row = slot.row;

				if (slot.immutable) {
						break;
					}
					// Immutability backstop (deferred remove path): same rationale as the
					// AddComponent branch — apply() migrates type-erased, bypassing
					// World::remove_component. No payload to free here.
					std::size_t drop_col_idx = NO_COLUMN_INDEX;
				{
					Archetype const& src = world.archetypes[src_idx];
					drop_col_idx = src.column_index_for(op.remove_id);
					if (drop_col_idx == NO_COLUMN_INDEX) {
						break; // entity doesn't carry it — silent no-op
					}
					if (src.signature.size() == 1) {
						// Removing the sole component is forbidden; mirror World::remove_component.
						break;
					}
				}

				std::vector<component_type_id_t> target_sig;
				std::vector<ColumnVTable const*> target_vtables;
				{
					Archetype const& src = world.archetypes[src_idx];
					target_sig.reserve(src.signature.size() - 1);
					target_vtables.reserve(src.signature.size() - 1);
					for (std::size_t i = 0; i < src.signature.size(); ++i) {
						if (src.signature[i] == op.remove_id) {
							continue;
						}
						target_sig.push_back(src.signature[i]);
						target_vtables.push_back(src.vtables[i]);
					}
				}

				uint32_t const target_idx
					= world.find_or_create_archetype(target_sig, target_vtables.data());

				Archetype::RowLocation target_loc = world.archetypes[target_idx].reserve_row();
				world.archetypes[target_idx].entity_array(target_loc.chunk_index)[target_loc.row] = eid;

				{
					Archetype& target = world.archetypes[target_idx];
					Archetype& src = world.archetypes[src_idx];
					if (src.column_offsets[drop_col_idx] != NO_COLUMN_OFFSET) {
						src.vtables[drop_col_idx]->destroy(
							src.row_in_column(src_chunk, drop_col_idx, src_row)
						);
					}
					for (std::size_t i = 0; i < target.signature.size(); ++i) {
						component_type_id_t const tid = target.signature[i];
						if (target.column_offsets[i] == NO_COLUMN_OFFSET) {
							continue;
						}
						std::size_t const src_col_idx = src.column_index_for(tid);
						void* dst = target.row_in_column(target_loc.chunk_index, i, target_loc.row);
						void* srcp = src.row_in_column(src_chunk, src_col_idx, src_row);
						target.vtables[i]->move_construct(dst, srcp);
					}
				}

				world.compact_archetype_after_external_move(src_idx, src_chunk, src_row);

				EntitySlot& mutable_slot = world.entity_slots[eid.index];
				mutable_slot.archetype_index = target_idx;
				mutable_slot.chunk_index = static_cast<uint32_t>(target_loc.chunk_index);
				mutable_slot.row = static_cast<uint32_t>(target_loc.row);
				break;
			}
		}
	}
	ops.clear();
	deferred_count_ = 0;
}

void CommandBuffer::clear() {
	// PayloadSlot destructors clean up any remaining values via their vtables.
	ops.clear();
	deferred_count_ = 0;
}

void CommandBuffer::merge_from(CommandBuffer&& other) {
	if (other.ops.empty()) {
		// Even with zero ops, fold deferred_count_ in case the caller cleared between recording
		// and merging (defensive — current uses don't hit this path).
		deferred_count_ += other.deferred_count_;
		other.deferred_count_ = 0;
		return;
	}
	ops.reserve(ops.size() + other.ops.size());
	uint32_t const base = deferred_count_;
	// Rebase incoming placeholder local_seqs by `base` so placeholders stay unique post-merge.
	// CreateEntity ops carry their own placeholder in op.eid; AddComponent / DestroyEntity /
	// RemoveComponent carry whatever placeholder the recorder passed in. All four kinds get
	// the same rewrite — `is_deferred()` is a pure flag check, so a real EID survives untouched.
	for (Op& op : other.ops) {
		if (op.eid.is_deferred()) {
			op.eid.index += base;
		}
		ops.push_back(std::move(op));
	}
	deferred_count_ += other.deferred_count_;
	other.ops.clear();
	other.deferred_count_ = 0;
}
