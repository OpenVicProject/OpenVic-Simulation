#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ChecksumTraits.hpp"
#include "openvic-simulation/ecs/Chunk.hpp"
#include "openvic-simulation/ecs/ChunkPool.hpp"
#include "openvic-simulation/ecs/ChunkView.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/System.hpp"

namespace OpenVic::ecs {
	class SystemScheduler; // forward — concrete type included from World.cpp only

	// Defined in Checksum.cpp only — read-only window over archetypes + singletons for the
	// full-state checksum walk. Deliberately a friend rather than a public accessor: nothing
	// else may walk the raw archetype vector.
	struct WorldChecksumAccess;
}

namespace OpenVic::ecs {

	// Sentinel marking a slot that has been reserved (so its EntityID is real and addressable)
	// but not yet finalised — the slot has no archetype/row and no components installed. Used
	// by `CommandBuffer::create_entity` so the caller can hold a usable EntityID before the
	// buffer is applied. While in this state, `is_alive(eid)` returns false.
	inline constexpr uint32_t INVALID_ARCHETYPE = static_cast<uint32_t>(-1);

	// One slot per ever-allocated entity. Free slots thread a singly-linked free-list through
	// `next_free`; `alive == false` marks them as dead. `generation` is bumped on each reuse so
	// stale EntityIDs reliably fail validity checks.
	//
	// `archetype_index == INVALID_ARCHETYPE` distinguishes the "reserved-but-unfinalised" state
	// (alive == true but has no archetype yet — only used during CommandBuffer recording).
	struct EntitySlot {
		uint32_t archetype_index = 0;
		uint32_t chunk_index = 0;
		uint32_t row = 0;
		uint32_t generation = 0;
		uint32_t next_free = 0;
		bool alive = false;
		// Set by create_immutable_entity / CommandBuffer::create_immutable_entity. While true,
		// World::add_component / remove_component (and the type-erased equivalents in
		// CommandBuffer::apply) refuse to migrate this entity — the runtime backstop behind the
		// ImmutableEntityID compile-time guarantee. Reset to false on slot reuse in
		// allocate_entity_slot. Lands in the bool's existing padding — no EntitySlot growth.
		bool immutable = false;
	};

	// Plain serializable image of the World's identity layer — the slot table that backs the
	// public EntityID contract: per-slot generations, per-slot immutability, and the free-list
	// order. Deliberately NOT captured: archetypes, chunks, rows, packing, singletons, systems —
	// those are rebuilt by the loader (recreate every live entity via World::restore_entity in
	// canonical slot-index order; packing may legitimately differ from the saved run and must
	// not matter). No IO/format here: callers serialize this struct however they like.
	struct WorldIdentitySnapshot {
		struct SlotRecord {
			// The slot's CURRENT stored generation. For a live slot this is the entity's
			// generation; for a free slot it is the last dead occupant's generation — the next
			// allocation bumps it (the bump happens at allocate time, see allocate_entity_slot),
			// so storing the current value reproduces the never-saved run's next-reuse
			// generation exactly.
			uint32_t generation = 0;
			// Live slots only. Normalized to false for free slots: a dead slot's immutable bit
			// is unobservable (allocate_entity_slot resets it on reuse), and copying leftover
			// garbage would make snapshots non-canonical (round-trip equality would depend on
			// unobservable history).
			bool immutable = false;

			bool operator==(SlotRecord const&) const = default;
		};

		// Indexed by slot index; size == entity_slots.size() at snapshot time.
		std::vector<SlotRecord> slots;
		// Free slots in pop order: free_list.front() == first_free (the slot the next
		// allocate_entity_slot reuses), free_list.back() == the chain tail (the self-pointing
		// next_free sentinel). A slot is free iff it appears here; every other slot is live.
		std::vector<uint32_t> free_list;

		bool operator==(WorldIdentitySnapshot const&) const = default;
	};

	// Hash for a sorted std::vector<component_type_id_t> used as an archetype-signature key.
	struct ArchetypeSignatureHash {
		std::size_t operator()(std::vector<component_type_id_t> const& sig) const noexcept {
			std::size_t h = sig.size();
			for (component_type_id_t id : sig) {
				// Mix high and low halves of the 64-bit id so signatures with the same low 32 bits
				// don't collide trivially.
				std::size_t mixed = static_cast<std::size_t>(id ^ (id >> 32));
				h ^= mixed + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
			}
			return h;
		}
	};

	// Cache key for a fully-built Query. Combines its sorted require-list and exclude-list.
	struct QueryCacheKey {
		std::vector<component_type_id_t> require_ids;
		std::vector<component_type_id_t> exclude_ids;

		bool operator==(QueryCacheKey const& other) const {
			return require_ids == other.require_ids && exclude_ids == other.exclude_ids;
		}
	};

	struct QueryCacheKeyHash {
		std::size_t operator()(QueryCacheKey const& key) const noexcept {
			ArchetypeSignatureHash h;
			std::size_t a = h(key.require_ids);
			std::size_t b = h(key.exclude_ids);
			return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
		}
	};

	struct CommandBuffer; // forward — friend below

	struct World {
		// Construct an entity with the given components. Each Cs... gets a column in the
		// resulting archetype; the components are aggregate-initialised from the supplied values.
		template<typename... Cs>
		EntityID create_entity(Cs&&... values);

		// Construct an IMMUTABLE entity: identical to create_entity but the returned handle is an
		// ImmutableEntityID that cannot be passed to add_component / remove_component (compile-time
		// guarantee), and the entity's slot is flagged so the type-erased structural paths refuse
		// to migrate it (runtime backstop). Component DATA stays mutable via get_component.
		template<typename... Cs>
		ImmutableEntityID create_immutable_entity(Cs&&... values);

		// === Bulk entity creation (ECS_SIM_ARCHITECTURE §9 item 4) ===
		// Creates `count` entities sharing one signature, paying the per-entity costs of the
		// equivalent create_entity loop once per batch: one signature sort, one archetype
		// lookup, chunk-granular row reservation (partial tail chunk filled first, then whole
		// chunks), column-contiguous construction, and one column-version bump per touched
		// chunk. The new EntityIDs are written to `out_ids` in creation order
		// (out_ids.size() must equal count).
		//
		// Per-entity component values arrive as one span per NON-EMPTY component, matched
		// positionally to the non-empty Cs... in pack order; tag components are named in the
		// pack but take no span. Each span's length must equal count. Pass no spans at all to
		// default-construct every component (tag-heavy / zeroed batches).
		//
		// Input spans are MOVED-FROM: values are move-constructed out of the caller's
		// storage, which is left holding moved-from husks (chosen over copying for the
		// 300k-pop session-setup path; pass a copy if the source must survive). Spans of
		// const elements are rejected at compile time so the contract cannot silently
		// degrade to copies.
		//
		// Determinism guarantee: a bulk create yields the IDENTICAL end state — including
		// identical EntityID assignment — as the equivalent create_entity loop. Entity slots
		// are allocated one-by-one in creation order (same free-list pops) and rows pack
		// exactly as N× reserve_row would. EntityID assignment feeds the per-entity RNG
		// stream (§8), so this equivalence is load-bearing; do not weaken it. The only
		// divergence from the loop is column_versions' numeric values (bumped once per
		// touched chunk instead of once per row) — versions only signal change, nothing
		// compares their values.
		//
		// Returns false (with an error log) when called mid-tick — out_ids is then filled
		// with INVALID_ENTITY_ID, matching the per-call EntityID {} the loop would return —
		// or when out_ids / any span length mismatches count (out_ids untouched).
		// count == 0 is a no-op returning true: like the empty loop, it does NOT create the
		// archetype (so archetype_epoch and the query cache are untouched).
		template<typename... Cs, typename... Spans>
		bool create_entities(std::size_t count, std::span<EntityID> out_ids, Spans&&... spans);

		// Bulk analogue of create_immutable_entity: identical to create_entities but every
		// created entity's slot is stamped immutable and the handles written to `out_ids`
		// are ImmutableEntityID (same compile-time structural-mutation guarantee as the
		// single-entity API).
		template<typename... Cs, typename... Spans>
		bool create_immutable_entities(
			std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
		);

		void destroy_entity(EntityID id);
		bool is_alive(EntityID id) const;

		// Returns true if and only if `id` refers to a LIVE entity that was created immutable (via
		// create_immutable_entity / CommandBuffer::create_immutable_entity). False for dead,
		// stale, or mutable entities. Useful when a system holds a plain EntityID (e.g. from
		// for_each_with_entity, where the compile-time guarantee no longer applies) and wants to
		// branch before queueing a structural op:
		//   if (!ctx.world.is_immutable(eid)) { ctx.cmd.add_component<C>(eid, ...); }
		bool is_immutable(EntityID id) const;

		template<typename C>
		C* get_component(EntityID id);

		template<typename C>
		C const* get_component(EntityID id) const;

		template<typename C>
		bool has_component(EntityID id) const;

		// === ImmutableEntityID read / data / destroy overloads ===
		// Reads return a mutable C* (data mutation on an immutable entity is allowed); destroy is
		// allowed (destroying a live entity is not an archetype mutation of it). These build an
		// EntityID inline from the handle's fields — NOT via unsafe_mutable_id(), so that name
		// stays exclusive to genuine structural-bypass sites. There is deliberately NO
		// add_component / remove_component overload for ImmutableEntityID.
		template<typename C>
		C* get_component(ImmutableEntityID id) {
			return get_component<C>(EntityID { id.index, id.generation });
		}

		template<typename C>
		C const* get_component(ImmutableEntityID id) const {
			return get_component<C>(EntityID { id.index, id.generation });
		}

		template<typename C>
		bool has_component(ImmutableEntityID id) const {
			return has_component<C>(EntityID { id.index, id.generation });
		}

		bool is_alive(ImmutableEntityID id) const {
			return is_alive(EntityID { id.index, id.generation });
		}

		void destroy_entity(ImmutableEntityID id) {
			destroy_entity(EntityID { id.index, id.generation });
		}

		bool is_immutable(ImmutableEntityID id) const {
			return is_immutable(EntityID { id.index, id.generation });
		}

		// Add a component to a living entity. Migrates the entity to the archetype that
		// extends its current one with C. If the entity already carries C, replaces the
		// existing value in place and returns the existing pointer. Returns nullptr if
		// the entity is dead.
		template<typename C>
		C* add_component(EntityID id, C&& value);

		// Default-construct overload — convenient for tag/empty types and for components
		// whose default value is the right initial state.
		template<typename C>
		C* add_component(EntityID id);

		// Remove a component from a living entity. Migrates the entity to the archetype
		// with C dropped. Returns false if the entity is dead, doesn't carry C, or removing
		// C would leave the entity with zero components (an invariant we don't allow —
		// callers should `destroy_entity` instead).
		template<typename C>
		bool remove_component(EntityID id);

		// Returns the component-column version for C in the entity's current archetype, or
		// 0 if the entity is dead / no longer carries C. The version monotonically
		// increases on every structural change to that column (push, swap-pop, relocate),
		// so a stable version implies cached pointers into the column are still valid.
		template<typename C>
		uint64_t component_version_in(EntityID id) const;

		template<typename C>
		uint64_t component_version_in(ImmutableEntityID id) const {
			return component_version_in<C>(EntityID { id.index, id.generation });
		}

		// Visit every entity whose archetype contains all of Cs..., calling fn(C&...) per row.
		template<typename... Cs, typename Fn>
		void for_each(Fn&& fn);

		// Same, but the function also receives the EntityID. Useful for collecting IDs to
		// destroy later (you can't destroy during iteration without invalidating columns).
		template<typename... Cs, typename Fn>
		void for_each_with_entity(Fn&& fn);

		// Query overloads — match archetypes against Query::require_ids and reject any that
		// overlap Query::exclude_ids. The lambda must accept C&... matching the call site's
		// `Cs...` template arguments (the exclude-set isn't reflected in the call signature).
		// Iterates only matched archetypes; results are cached per (require, exclude) key
		// and invalidated whenever a new archetype is created.
		template<typename... Cs, typename Fn>
		void for_each(Query const& query, Fn&& fn);

		template<typename... Cs, typename Fn>
		void for_each_with_entity(Query const& query, Fn&& fn);

		// Chunk-granularity iteration. The lambda receives a `ChunkView<Cs...>` exposing
		// raw entity-id and component slabs of length `view.count()`. Use this when an
		// inner loop wants tight, function-call-free access to component arrays
		// (SIMD-friendly because slabs are contiguous and aligned).
		template<typename... Cs, typename Fn>
		void for_each_chunk(Fn&& fn);

		template<typename... Cs, typename Fn>
		void for_each_chunk(Query const& query, Fn&& fn);

		// === Singletons ===
		// Type-erased per-type unique slot, owned by the World. Lifetime is the World's
		// lifetime; not cleared by `clear_systems` or `end_game_session`. Singletons are
		// the right home for global simulation state that doesn't belong on a particular
		// entity (e.g. a clock, a registry, a per-session config blob).
		//
		// Scheduler contract: singletons share component_type_id_t with components, but they
		// are NOT visible in any tick signature — a system that touches one inside its tick
		// MUST declare it, or the scheduler may co-schedule it against a conflicting system
		// and silently break worker-count-invariant determinism:
		//   - `extra_reads()  = { component_type_id_of<C>() }` for get_singleton reads;
		//   - `extra_writes() = { component_type_id_of<C>() }` for mutation through the
		//     returned pointer (serial systems only — see register_system's static_assert).
		// Creating a NEW singleton mid-tick (set_singleton on a missing id) mutates the
		// `singletons` hashmap and can rehash under a concurrent reader even with correct
		// declarations — create every singleton before the first `tick_systems` call.
		template<typename C>
		C* set_singleton(C&& value);

		template<typename C>
		C* set_singleton(); // default-construct

		template<typename C>
		C* get_singleton();

		template<typename C>
		C const* get_singleton() const;

		template<typename C>
		bool clear_singleton();

		// === System registration ===
		// Systems are templated by their concrete derived type. The World stores a
		// type-erased SystemRegistration plus an owned pending CommandBuffer per system.
		// On the first `tick_systems` call after registration, the SystemScheduler builds
		// a DAG (declared deps + auto-resolved access conflicts) and a stable topological
		// schedule, then drives execution. Cross-machine deterministic given identical
		// registrations.
		template<typename S, typename... Args>
		SystemHandle register_system(Args&&... args);

		void unregister_system(SystemHandle handle);
		void tick_systems(TickContext const& ctx);
		void tick_systems(Date today);
		void clear_systems();

		// FNV-1a hash over the (stage_index, system_type_id_t) pairs of the current
		// schedule. Multiplayer peers compute this at session-start handshake; mismatch
		// rejects the join.
		uint64_t schedule_hash();

		// Test/introspection only: stage layout of the current schedule (forces a rebuild if
		// dirty, exactly like schedule_hash). `debug_stage_index_of` returns SIZE_MAX when the
		// system isn't scheduled. Tests use these to assert two systems do (or do not) share a
		// stage — the observable effect of the disjoint-iteration conflict override.
		std::size_t debug_stage_count();
		std::size_t debug_stage_index_of(system_type_id_t type_id);

		// Force the scheduler to run every stage on the calling thread. Used for tests
		// to validate "parallel result == serial result". Default false.
		void set_serial_mode(bool enabled);

		// Returns the EcsThreadPool used by the scheduler. Lazily constructed with the
		// default worker count on first access.
		EcsThreadPool& ecs_thread_pool();

		// Pointer to the SystemRegistration currently being driven by the scheduler.
		// Used by SystemThreaded::tick_all to access its pending_cmd. Returns nullptr
		// outside `tick_systems` execution.
		SystemRegistration* current_system_registration();

		// Internal: set by SystemScheduler around each system's tick. Public to keep the
		// scheduler decoupled from World's privates; not intended for general use.
		void set_current_registration_(SystemRegistration* reg) {
			current_system_registration_ = reg;
		}

		// Internal: set by SystemScheduler around the per-stage CommandBuffer apply loop.
		// While true, the in-tick mutation guard yields so cmd.destroy_entity /
		// cmd.add_component / cmd.remove_component / cmd.create_entity executed via
		// CommandBuffer::apply actually mutate the World. Not intended for general use.
		void set_in_apply_phase_(bool value) {
			in_apply_phase_ = value;
		}

		// Public chunk-walk helpers used by SystemThreaded's per-chunk parallel iteration.
		std::size_t archetype_chunk_count(uint32_t archetype_idx) const;
		std::size_t archetype_chunk_row_count(uint32_t archetype_idx, std::size_t chunk_idx) const;

		struct CachedQuery {
			uint32_t epoch = 0;
			uint64_t require_matcher = 0;
			uint64_t exclude_matcher = 0;
			std::vector<uint32_t> archetype_indices;
		};

		// Public lookup for SystemThreaded — same body as the private `resolve_query_cache`.
		CachedQuery const& resolve_query_cache_for_threaded(QueryCacheKey const& key) const;

		// Iterate one chunk's rows for a Cs... query — used by SystemThreaded's parallel_for
		// body. The body is invoked once per row with `(Cs&...)`.
		template<typename... Cs, typename Body>
		void iterate_one_chunk_for_threaded(uint32_t archetype_idx, uint32_t chunk_idx, Body&& body);

		template<typename... Cs, typename Body>
		void iterate_one_chunk_with_entity_for_threaded(
			uint32_t archetype_idx, uint32_t chunk_idx, Body&& body);

		// === Reserved-but-unfinalised slot ===
		// Reserves an entity slot without placing it in any archetype. The returned EntityID
		// is real (its index/generation are addressable), but `is_alive` returns false until
		// the slot is finalised — typical use is `CommandBuffer::create_entity`, which calls
		// this synchronously and finalises it later via `apply()`. Safe to call from anywhere
		// the World is reachable; threading-safe variants will come when threading lands.
		EntityID reserve_entity_slot();

		// Finalise a previously reserved slot by inserting it into the archetype matching
		// `sorted_sig` (sorted ascending). Components are move-constructed from `slots`
		// using the matching ColumnVTable; `slots[i]` must be non-null for non-tag columns
		// and ignored for tag columns. After this returns, `is_alive(eid) == true`.
		// Caller is responsible for ensuring `eid` is a reserved slot (alive but
		// archetype_index == INVALID_ARCHETYPE).
		void finalize_reserved_entity(
			EntityID eid,
			std::vector<component_type_id_t> const& sorted_sig,
			std::vector<ColumnVTable const*> const& sorted_vtables,
			std::vector<void*> const& sorted_value_slots
		);

		// Bulk analogue of finalize_reserved_entity, used by CommandBuffer::apply for batch
		// create ops. `column_blocks` is parallel to `sorted_sig`: each non-tag entry points
		// at a contiguous block of ids.size() staged values (stride == vtable size); tag
		// entries are nullptr. Values are move-constructed out of the blocks (sources
		// destroyed via ColumnVTable::move_construct_n) — after this returns the caller frees
		// the raw block memory WITHOUT running element destructors again.
		//
		// When every id passes the reserved-but-unfinalised checks (the only state reachable
		// without misuse, and guaranteed on the parallel CommandBuffer path where slots are
		// reserved during apply), the batch takes the fast path: one archetype lookup, bulk
		// row reservation, column-contiguous move_construct_n. If ANY id fails (e.g. the
		// caller destroyed a serial-reserved id between record and apply), the whole batch
		// falls back to per-entity finalize_reserved_entity so only the bad slots are skipped
		// — exactly the loop-of-single-creates semantics — and the skipped entities' staged
		// values are destroyed here. Immutability stamping is the caller's job (as with
		// finalize_reserved_entity).
		void finalize_reserved_entities_bulk(
			std::span<EntityID const> ids,
			std::vector<component_type_id_t> const& sorted_sig,
			std::vector<ColumnVTable const*> const& sorted_vtables,
			std::span<void* const> column_blocks
		);

		// Drops a reserved-but-unfinalised slot (used when CommandBuffer records a destroy
		// for an entity it previously reserved, before `apply()` had a chance to finalise it).
		// No-op for slots that are already finalised or already dead.
		void drop_reserved_slot(EntityID eid);

		// === Identity-layer save/load (EntityID save-stability — ECS_SIM_ARCHITECTURE §9 item 3) ===

		// Captures the identity layer ONLY (see WorldIdentitySnapshot): per-slot generations,
		// per-slot immutability, free-list order. Refuses (error log + false, `out` left cleared)
		// when called mid-tick, or while any reserved-but-unfinalised slot exists — i.e. a
		// CommandBuffer holding un-applied creates. Snapshot only between ticks, after every
		// buffer has been applied. Free-chain corruption (cycle, live node on the chain, dead
		// slot unreachable from the chain) is detected and also fails loudly — better to refuse
		// a save than to diverge k ticks after load.
		bool snapshot_identity(WorldIdentitySnapshot& out) const;

		// Rebuilds the identity layer from a snapshot. Precondition: a FRESH World (no entity
		// slot ever allocated) outside any tick; the snapshot is validated fully before any
		// mutation, so on failure (error log + false) the World is untouched.
		//
		// Postcondition: every live slot is reserved-but-unfinalised — addressable at its
		// original (index, generation) but `is_alive == false` until restore_entity finalises
		// it; free slots carry their restored generations and the exact saved chain order, so
		// subsequent allocations continue exactly as in the never-saved run (generation bumps
		// happen at allocate time, identically in both runs).
		//
		// Loader contract:
		//   1. Between restore_identity and the last restore_entity call, the ONLY legal entity
		//      ops are restore_entity calls, one per live slot. In particular destroy_entity on
		//      a not-yet-finalised id routes to drop_reserved_slot and would PUSH the slot onto
		//      the free list, silently corrupting the restored order.
		//   2. Canonical recreation order is slot-index ascending. Identity correctness is
		//      order-independent, but packing is not — the canonical order is what makes packing
		//      reproducible across loads.
		//   3. The identity layer guarantees same-allocation-request-sequence → same ids.
		//      Producing the same request sequence post-load is the CALLER's obligation:
		//      chunk-order-driven cmd.create_entity over an archetype whose packing differs from
		//      the saved run WILL assign different ids (§10 — anything id-assignment-sensitive
		//      must iterate in id / dense-index order, never chunk order).
		//   4. Query caches and the chunk pool need no attention: caches rebuild lazily via
		//      archetype_epoch as restore_entity creates archetypes, and pool aging affects
		//      memory residency only, never simulation results.
		bool restore_identity(WorldIdentitySnapshot const& snapshot);

		// Finalises ONE restored slot with the given components — the loader-facing half of
		// restore_identity (same component rules as create_entity). Validates that `eid` names
		// a reserved-but-unfinalised slot with a matching generation; returns false + error log
		// on any mismatch (deferred id, out of range, dead slot, stale generation, already
		// finalised). Not callable mid-tick.
		//
		// Preserves the slot's restored `immutable` flag: finalization IS creation here, and the
		// immutability guarantee is "no archetype change AFTER creation". There is deliberately
		// no ImmutableEntityID overload — immutability is restored as data by restore_identity,
		// not by how this is called; rebuild strong handles as ImmutableEntityID { index,
		// generation } after finalization.
		template<typename... Cs>
		bool restore_entity(EntityID eid, Cs&&... values);

	public:
		// Ctor / dtor are out-of-line — they must instantiate destructors of unique_ptr
		// fields whose pointee types (SystemScheduler) are forward-declared at this point.
		World();
		~World();
		World(World const&) = delete;
		World& operator=(World const&) = delete;
		World(World&&) = delete;
		World& operator=(World&&) = delete;

		// Override the ECS worker count. Call before the first `tick_systems` invocation.
		// 0 → defaults to hw_concurrency, capped at 16.
		void set_ecs_worker_count(uint32_t count);

		// Direct access to the per-World chunk pool. Tests use this to inspect pool state
		// (pooled_count / total_allocations / total_deallocations / current_tick). Production
		// code does not need to call this — archetypes hold an internal pointer.
		ChunkPool& chunk_pool() {
			return chunk_pool_;
		}
		ChunkPool const& chunk_pool() const {
			return chunk_pool_;
		}

	private:
		std::vector<EntitySlot> entity_slots;
		uint32_t first_free = 0;
		bool has_free = false;

		// Declared before `archetypes` so the destruction order is `archetypes` (which drain
		// their chunks into the pool via the World destructor's explicit drain loop) first,
		// then `chunk_pool_` (which frees the cached blocks). Don't reorder.
		ChunkPool chunk_pool_;
		std::vector<Archetype> archetypes;
		std::unordered_map<std::vector<component_type_id_t>, uint32_t, ArchetypeSignatureHash> archetype_by_signature;

		// Bumped every time `find_or_create_archetype` actually inserts a new archetype.
		// Used to invalidate the query cache lazily.
		uint32_t archetype_epoch = 0;

		mutable std::unordered_map<QueryCacheKey, CachedQuery, QueryCacheKeyHash> query_cache;

		// Type-erased system registry (replaces the old virtual-System SystemSlot storage).
		// Each entry owns its system instance + pending CommandBuffer. Indices are stable;
		// unregistered entries set `alive=false` and bump generation.
		std::vector<SystemRegistration> system_registry_;

		// Owned pending CommandBuffer per system. Parallel array to system_registry_; the
		// SystemRegistration's `pending_cmd` pointer points here. Owned via unique_ptr so
		// the CB itself is stable across vector growth (registry vector growth never moves
		// the CommandBuffer storage).
		std::vector<std::unique_ptr<CommandBuffer>> pending_cmds_;

		// Set true by the scheduler around each system's tick(). Asserted by the four
		// structural-mutation methods (create_entity, destroy_entity, add_component,
		// remove_component) so direct mutations during a tick are caught — only path is
		// `ctx.cmd` or per-chunk buffer in SystemThreaded.
		bool in_tick_ = false;

		// Set true by the scheduler around the per-stage CommandBuffer apply loop. While
		// in_apply_phase_ is true the in_tick_or_log_ guard yields, so the type-erased
		// destroy/add/remove ops queued via ctx.cmd actually execute. Outside of apply
		// the guard still blocks direct structural mutations during a tick. Without this
		// flag, every cmd.destroy_entity / cmd.add_component / cmd.remove_component would
		// silently fail at apply time because in_tick_ is still set for the duration of
		// scheduler_->run().
		bool in_apply_phase_ = false;

		// Pointer to the SystemRegistration currently being driven. Used by
		// SystemThreaded::tick_all to access its pending_cmd. Set/cleared by the scheduler.
		SystemRegistration* current_system_registration_ = nullptr;

		// Forced-serial mode for tests: scheduler runs every stage on the caller's thread
		// in deterministic order regardless of inter-system parallelism.
		bool serial_mode_ = false;

		// EcsThreadPool — owned. Lazily constructed on first `ecs_thread_pool()` access.
		std::unique_ptr<EcsThreadPool> ecs_thread_pool_;
		uint32_t ecs_worker_count_ = 0; // 0 = use default at construction time

		// Scheduler — owned. Lazily constructed on first `tick_systems`. Holds the DAG
		// + stage layout + schedule_hash. Forward-declared; full type in SystemScheduler.hpp.
		std::unique_ptr<class SystemScheduler> scheduler_;

		// Tracks whether scheduler_ needs a rebuild (after register_system / unregister_system
		// / clear_systems). Lazy build happens on first `tick_systems` after invalidation.
		bool scheduler_dirty_ = true;

		// In-tick mutation guard helper. Returns true — after logging an error naming the
		// refused operation — if `in_tick_` is set and we are not in the stage-barrier apply
		// phase; the caller early-returns its failure value. Returns false otherwise.
		bool in_tick_or_log_(char const* fn_name);

		// Immutability backstop helper. Returns true (and logs an error naming the operation,
		// entity and component) if `id`'s slot is flagged immutable, so the structural mutators
		// can early-return without migrating. Callers must only invoke after is_alive(id) has
		// proven the index in-range, alive and finalised. Const: reads the slot only.
		bool immutable_or_log_(EntityID id, char const* fn_name, std::string_view component_name) const;

		// Validation + logging for restore_entity, out-of-line so the header template stays free
		// of the Logger include. True iff `eid` names a reserved-but-unfinalised slot with a
		// matching generation; logs an error naming the failure otherwise.
		bool restore_entity_precheck_(EntityID eid) const;

		// Appends a fresh pending CommandBuffer to `pending_cmds_` and returns a stable pointer
		// to it. Out-of-line (defined in World.cpp, where CommandBuffer is complete) so the
		// register_system template does NOT force `make_unique<CommandBuffer>` / the
		// vector<unique_ptr<CommandBuffer>> element-destroy path — both of which need the
		// complete type — into every translation unit that instantiates register_system. Keeps
		// the CB heap-stable across registry growth (see the `pending_cmds_` member comment).
		CommandBuffer* allocate_pending_cmd_();

		// Shared body of create_entity / create_immutable_entity. `immutable` is stamped onto the
		// new entity's slot. Reused by both wrappers so the sort + placement-new fold lives once.
		template<typename... Cs>
		EntityID create_entity_impl(bool immutable, Cs&&... values);

		// Shared body of create_entities / create_immutable_entities. OutIdT is EntityID or
		// ImmutableEntityID (both aggregate-init from { index, generation }).
		template<typename OutIdT, typename... Cs, typename... Spans>
		bool create_entities_impl(
			bool immutable, std::size_t count, std::span<OutIdT> out_ids, Spans&&... spans
		);

		// Out-of-line validation + logging for the bulk-create entry points, so the header
		// template stays free of the Logger include. True iff out_ids_size == count and every
		// span_sizes[0..span_count) == count; logs an error naming the mismatch otherwise.
		bool bulk_create_sizes_ok_(
			std::size_t count, std::size_t out_ids_size, std::size_t const* span_sizes,
			std::size_t span_count, char const* fn_name
		) const;

		// Reused scratch for bulk row reservation (create_entities_impl /
		// finalize_reserved_entities_bulk) — one allocation amortised across batches.
		std::vector<Archetype::RowRange> bulk_rows_scratch_;

		friend struct WorldChecksumAccess;

		// Singletons. Type-erased deleter calls `delete static_cast<C*>(p)` so the World
		// destructor sweeps them automatically. The checksum thunk is captured at
		// set_singleton<C> time — the map has no other type metadata, so this function
		// pointer is the only record of how to hash the value (Checksum.cpp walk).
		using SingletonPtr = std::unique_ptr<void, void (*)(void*)>;
		struct SingletonRecord {
			SingletonPtr ptr;
			checksum_value_fn checksum;
		};
		std::unordered_map<component_type_id_t, SingletonRecord> singletons;

		// Allocates a new entity slot and returns the resulting EntityID. Slot is marked alive
		// but its archetype_index/row are not yet meaningful — caller fills those.
		EntityID allocate_entity_slot();

		// Looks up an existing archetype matching `sig` (sorted) or creates one with empty
		// chunks (none allocated until `reserve_row` is called). Returns the archetype's
		// index in `archetypes`. Bumps `archetype_epoch` if a new archetype was created.
		uint32_t find_or_create_archetype(
			std::vector<component_type_id_t> const& sig, ColumnVTable const* const* vtables
		);

		// Computes column_offsets and chunk_capacity for an archetype. Tag columns receive
		// NO_COLUMN_OFFSET. Returns the per-row total bytes (used internally; callers don't
		// usually need it).
		void compute_chunk_layout(Archetype& arch);

		// Computes the matcher_hash bitfield (1 bit per component, derived from id % 63).
		static uint64_t compute_matcher_hash(std::vector<component_type_id_t> const& sig) {
			uint64_t mask = 0;
			for (component_type_id_t id : sig) {
				mask |= (uint64_t { 1 } << (id % 63));
			}
			return mask;
		}

		// After a row is "removed" — either destroyed in place (destroy_entity) or moved
		// out (migration) — compact the archetype by swap-popping with the global last row.
		// If the removed row is itself the global last row, this just decrements; otherwise
		// every column is move-constructed from (last_chunk, last_row) into (chunk, row),
		// the relocated entity's slot is updated, and the last chunk's count drops.
		// Bumps every column_version. Drops a trailing chunk if it becomes empty.
		void compact_archetype_after_external_move(uint32_t archetype_index, std::size_t chunk_index, std::size_t row);

		// Rebuild a query-cache entry by walking every archetype.
		CachedQuery const& resolve_query_cache(QueryCacheKey const& key) const;

		friend struct CommandBuffer;
	};

	// === template definitions ===

	template<typename... Cs>
	EntityID World::create_entity_impl(bool immutable, Cs&&... values) {
		static_assert(sizeof...(Cs) > 0, "create_entity requires at least one component");

		if (in_tick_or_log_("World::create_entity")) {
			return EntityID {};
		}

		// Build the sorted signature and a parallel vtable list, indexed by the sorted order.
		component_type_id_t const raw_ids[] = { component_type_id_of<std::remove_cvref_t<Cs>>()... };
		ColumnVTable const* const raw_vtables[] = { &column_vtable_for<std::remove_cvref_t<Cs>>()... };
		constexpr std::size_t const N = sizeof...(Cs);

		component_type_id_t sorted_ids[N];
		ColumnVTable const* sorted_vtables[N];
		for (std::size_t i = 0; i < N; ++i) {
			sorted_ids[i] = raw_ids[i];
			sorted_vtables[i] = raw_vtables[i];
		}
		// Tiny N — bubble sort is fine.
		for (std::size_t i = 0; i < N; ++i) {
			for (std::size_t j = i + 1; j < N; ++j) {
				if (sorted_ids[j] < sorted_ids[i]) {
					std::swap(sorted_ids[i], sorted_ids[j]);
					std::swap(sorted_vtables[i], sorted_vtables[j]);
				}
			}
		}

		std::vector<component_type_id_t> sig(sorted_ids, sorted_ids + N);
		uint32_t const archetype_idx = find_or_create_archetype(sig, sorted_vtables);

		EntityID const eid = allocate_entity_slot();

		// Reserve a row in the archetype.
		Archetype::RowLocation loc = archetypes[archetype_idx].reserve_row();
		archetypes[archetype_idx].entity_array(loc.chunk_index)[loc.row] = eid;

		// Construct each provided component into its column slot, finding the column by raw id.
		auto place = [&]<typename C>(C&& value) {
			using TC = std::remove_cvref_t<C>;
			component_type_id_t const id = component_type_id_of<TC>();
			std::size_t const col = archetypes[archetype_idx].column_index_for(id);
			if constexpr (!std::is_empty_v<TC>) {
				void* slot = archetypes[archetype_idx].row_in_column(loc.chunk_index, col, loc.row);
				::new (slot) TC(std::forward<C>(value));
			} else {
				(void) col;
				(void) value;
			}
		};
		(place(std::forward<Cs>(values)), ...);

		entity_slots[eid.index].archetype_index = archetype_idx;
		entity_slots[eid.index].chunk_index = static_cast<uint32_t>(loc.chunk_index);
		entity_slots[eid.index].row = static_cast<uint32_t>(loc.row);
		entity_slots[eid.index].immutable = immutable;
		return eid;
	}

	template<typename... Cs>
	EntityID World::create_entity(Cs&&... values) {
		return create_entity_impl<Cs...>(false, std::forward<Cs>(values)...);
	}

	template<typename... Cs>
	ImmutableEntityID World::create_immutable_entity(Cs&&... values) {
		EntityID const eid = create_entity_impl<Cs...>(true, std::forward<Cs>(values)...);
		return ImmutableEntityID { eid.index, eid.generation };
	}

	namespace detail {
		// Number of non-empty (data-carrying) components in Cs... — the number of input
		// spans the bulk-create APIs expect.
		template<typename... Cs>
		constexpr std::size_t non_empty_component_count() {
			return (static_cast<std::size_t>(!std::is_empty_v<Cs>) + ... + std::size_t { 0 });
		}

		// Maps pack index -> position in the span list (counting only non-empty components);
		// SIZE_MAX for tag components, which take no span.
		template<typename... Cs>
		constexpr std::array<std::size_t, sizeof...(Cs)> bulk_span_index_map() {
			std::array<std::size_t, sizeof...(Cs)> map {};
			constexpr bool is_tag[] = { std::is_empty_v<Cs>... };
			std::size_t next = 0;
			for (std::size_t i = 0; i < sizeof...(Cs); ++i) {
				map[i] = is_tag[i] ? static_cast<std::size_t>(-1) : next++;
			}
			return map;
		}

		// std::tuple<std::span<C>...> over the non-empty components of Cs..., in pack order.
		// Element types are non-const so a std::span<C const> (or const container) argument
		// fails to convert — the moved-from input contract cannot silently become a copy.
		template<typename... Cs>
		using BulkSpanTuple = decltype(std::tuple_cat(std::declval<
			std::conditional_t<std::is_empty_v<Cs>, std::tuple<>, std::tuple<std::span<Cs>>>
		>()...));
	}

	template<typename OutIdT, typename... Cs, typename... Spans>
	bool World::create_entities_impl(
		bool immutable, std::size_t count, std::span<OutIdT> out_ids, Spans&&... spans
	) {
		static_assert(sizeof...(Cs) > 0, "create_entities requires at least one component");
		static_assert(
			(std::is_same_v<Cs, std::remove_cvref_t<Cs>> && ...),
			"create_entities component types must be plain types (no const/volatile/reference)"
		);
		constexpr std::size_t const non_empty = detail::non_empty_component_count<Cs...>();
		static_assert(
			sizeof...(Spans) == non_empty || sizeof...(Spans) == 0,
			"create_entities takes one span per non-empty component (matched to the non-empty "
			"Cs... in pack order; tags take no span), or no spans to default-construct"
		);
		constexpr bool const use_spans = sizeof...(Spans) > 0;

		// Convert the deduced arguments to typed spans up-front (tags excluded). With no
		// spans supplied this is a tuple of empty spans, unused by the construction loop.
		detail::BulkSpanTuple<Cs...> typed_spans;
		if constexpr (use_spans) {
			typed_spans = detail::BulkSpanTuple<Cs...> { std::span(std::forward<Spans>(spans))... };
		}

		if (in_tick_or_log_("World::create_entities")) {
			// Match the loop's observable output: each blocked create_entity call returns
			// EntityID {}.
			std::fill(out_ids.begin(), out_ids.end(), OutIdT {});
			return false;
		}

		if constexpr (use_spans) {
			std::size_t span_sizes[non_empty];
			std::size_t si = 0;
			std::apply(
				[&](auto const&... s) {
					((span_sizes[si++] = s.size()), ...);
				},
				typed_spans
			);
			if (!bulk_create_sizes_ok_(count, out_ids.size(), span_sizes, non_empty, "World::create_entities")) {
				return false;
			}
		} else {
			if (!bulk_create_sizes_ok_(count, out_ids.size(), nullptr, 0, "World::create_entities")) {
				return false;
			}
		}

		if (count == 0) {
			// The equivalent empty loop creates nothing — in particular it does NOT create
			// the archetype, so neither do we (find_or_create_archetype would bump
			// archetype_epoch, an observable end-state divergence).
			return true;
		}

		// Sorted signature + parallel vtable list — same fold as create_entity_impl, paid
		// once per batch.
		component_type_id_t const raw_ids[] = { component_type_id_of<Cs>()... };
		ColumnVTable const* const raw_vtables[] = { &column_vtable_for<Cs>()... };
		constexpr std::size_t const N = sizeof...(Cs);

		component_type_id_t sorted_ids[N];
		ColumnVTable const* sorted_vtables[N];
		for (std::size_t i = 0; i < N; ++i) {
			sorted_ids[i] = raw_ids[i];
			sorted_vtables[i] = raw_vtables[i];
		}
		for (std::size_t i = 0; i < N; ++i) {
			for (std::size_t j = i + 1; j < N; ++j) {
				if (sorted_ids[j] < sorted_ids[i]) {
					std::swap(sorted_ids[i], sorted_ids[j]);
					std::swap(sorted_vtables[i], sorted_vtables[j]);
				}
			}
		}

		std::vector<component_type_id_t> sig(sorted_ids, sorted_ids + N);
		uint32_t const archetype_idx = find_or_create_archetype(sig, sorted_vtables);

		// No further archetype creation below — this reference stays valid.
		Archetype& arch = archetypes[archetype_idx];
		arch.reserve_rows(count, bulk_rows_scratch_);

		// Allocate entity slots one-by-one in creation order: identical free-list pops to
		// the create_entity loop, which is what keeps bulk id assignment bit-identical to
		// the loop (EntityID assignment feeds the RNG stream — ECS_SIM_ARCHITECTURE §8).
		{
			std::size_t i = 0;
			for (Archetype::RowRange const& range : bulk_rows_scratch_) {
				EntityID* const earr = arch.entity_array(range.chunk_index);
				for (std::size_t k = 0; k < range.count; ++k, ++i) {
					EntityID const eid = allocate_entity_slot();
					earr[range.row_begin + k] = eid;
					EntitySlot& slot = entity_slots[eid.index];
					slot.archetype_index = archetype_idx;
					slot.chunk_index = static_cast<uint32_t>(range.chunk_index);
					slot.row = static_cast<uint32_t>(range.row_begin + k);
					slot.immutable = immutable;
					out_ids[i] = OutIdT { eid.index, eid.generation };
				}
			}
		}

		// Column-contiguous construction: per non-empty component, per row range, a typed
		// move-construct (or default-construct) loop straight into the slab — no vtable
		// dispatch on this path.
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			constexpr std::array<std::size_t, N> span_map = detail::bulk_span_index_map<Cs...>();
			auto construct_column = [&]<std::size_t I>() {
				using TC = std::tuple_element_t<I, std::tuple<Cs...>>;
				if constexpr (!std::is_empty_v<TC>) {
					std::size_t const col = arch.column_index_for(component_type_id_of<TC>());
					std::size_t offset = 0;
					for (Archetype::RowRange const& range : bulk_rows_scratch_) {
						TC* const dst = static_cast<TC*>(
							arch.row_in_column(range.chunk_index, col, range.row_begin)
						);
						if constexpr (use_spans) {
							std::span<TC> const src = std::get<span_map[I]>(typed_spans);
							for (std::size_t k = 0; k < range.count; ++k) {
								::new (dst + k) TC(std::move(src[offset + k]));
							}
						} else {
							for (std::size_t k = 0; k < range.count; ++k) {
								::new (dst + k) TC {};
							}
						}
						offset += range.count;
					}
				}
			};
			(construct_column.template operator()<Is>(), ...);
		}(std::index_sequence_for<Cs...> {});

		return true;
	}

	template<typename... Cs, typename... Spans>
	bool World::create_entities(std::size_t count, std::span<EntityID> out_ids, Spans&&... spans) {
		return create_entities_impl<EntityID, Cs...>(false, count, out_ids, std::forward<Spans>(spans)...);
	}

	template<typename... Cs, typename... Spans>
	bool World::create_immutable_entities(
		std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
	) {
		return create_entities_impl<ImmutableEntityID, Cs...>(true, count, out_ids, std::forward<Spans>(spans)...);
	}

	template<typename... Cs>
	bool World::restore_entity(EntityID eid, Cs&&... values) {
		static_assert(sizeof...(Cs) > 0, "restore_entity requires at least one component");

		if (in_tick_or_log_("World::restore_entity")) {
			return false;
		}
		if (!restore_entity_precheck_(eid)) {
			return false;
		}

		// Build the sorted signature and a parallel vtable list — same fold as create_entity_impl.
		component_type_id_t const raw_ids[] = { component_type_id_of<std::remove_cvref_t<Cs>>()... };
		ColumnVTable const* const raw_vtables[] = { &column_vtable_for<std::remove_cvref_t<Cs>>()... };
		constexpr std::size_t const N = sizeof...(Cs);

		component_type_id_t sorted_ids[N];
		ColumnVTable const* sorted_vtables[N];
		for (std::size_t i = 0; i < N; ++i) {
			sorted_ids[i] = raw_ids[i];
			sorted_vtables[i] = raw_vtables[i];
		}
		for (std::size_t i = 0; i < N; ++i) {
			for (std::size_t j = i + 1; j < N; ++j) {
				if (sorted_ids[j] < sorted_ids[i]) {
					std::swap(sorted_ids[i], sorted_ids[j]);
					std::swap(sorted_vtables[i], sorted_vtables[j]);
				}
			}
		}

		// Materialise the forwarded values; finalize_reserved_entity move-constructs FROM these
		// slots into the archetype slabs, and the tuple destructor cleans up the moved-from
		// husks. Tag (empty) types keep a nullptr value slot — finalize skips size == 0 columns.
		std::tuple<std::remove_cvref_t<Cs>...> staged { std::forward<Cs>(values)... };

		std::vector<component_type_id_t> sig(sorted_ids, sorted_ids + N);
		std::vector<ColumnVTable const*> vtables(sorted_vtables, sorted_vtables + N);
		std::vector<void*> value_slots(N, nullptr);
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			auto stage = [&]<std::size_t I>() {
				using TC = std::remove_cvref_t<std::tuple_element_t<I, std::tuple<Cs...>>>;
				if constexpr (!std::is_empty_v<TC>) {
					component_type_id_t const id = component_type_id_of<TC>();
					for (std::size_t i = 0; i < N; ++i) {
						if (sig[i] == id) {
							value_slots[i] = static_cast<void*>(&std::get<I>(staged));
							break;
						}
					}
				}
			};
			(stage.template operator()<Is>(), ...);
		}(std::index_sequence_for<Cs...> {});

		finalize_reserved_entity(eid, sig, vtables, value_slots);
		return true;
	}

	template<typename C>
	C* World::get_component(EntityID id) {
		if (!is_alive(id)) {
			return nullptr;
		}
		EntitySlot const& slot = entity_slots[id.index];
		Archetype& arch = archetypes[slot.archetype_index];
		std::size_t const col = arch.column_index_for(component_type_id_of<C>());
		if (col == NO_COLUMN_INDEX) {
			return nullptr;
		}
		if constexpr (std::is_empty_v<C>) {
			return nullptr;
		} else {
			return static_cast<C*>(arch.row_in_column(slot.chunk_index, col, slot.row));
		}
	}

	template<typename C>
	C const* World::get_component(EntityID id) const {
		if (!is_alive(id)) {
			return nullptr;
		}
		EntitySlot const& slot = entity_slots[id.index];
		Archetype const& arch = archetypes[slot.archetype_index];
		std::size_t const col = arch.column_index_for(component_type_id_of<C>());
		if (col == NO_COLUMN_INDEX) {
			return nullptr;
		}
		if constexpr (std::is_empty_v<C>) {
			return nullptr;
		} else {
			return static_cast<C const*>(arch.row_in_column(slot.chunk_index, col, slot.row));
		}
	}

	template<typename C>
	bool World::has_component(EntityID id) const {
		if (!is_alive(id)) {
			return false;
		}
		EntitySlot const& slot = entity_slots[id.index];
		Archetype const& arch = archetypes[slot.archetype_index];
		return arch.has_component(component_type_id_of<C>());
	}

	template<typename C>
	uint64_t World::component_version_in(EntityID id) const {
		if (!is_alive(id)) {
			return 0;
		}
		EntitySlot const& slot = entity_slots[id.index];
		Archetype const& arch = archetypes[slot.archetype_index];
		std::size_t const col = arch.column_index_for(component_type_id_of<C>());
		if (col == NO_COLUMN_INDEX) {
			return 0;
		}
		return arch.column_versions[col];
	}

	template<typename C>
	C* World::add_component(EntityID id, C&& value) {
		using TC = std::remove_cvref_t<C>;

		if (in_tick_or_log_("World::add_component")) {
			return nullptr;
		}
		if (!is_alive(id)) {
			return nullptr;
		}
		if (immutable_or_log_(id, "World::add_component", ComponentName<TC>::value)) {
			return nullptr;
		}

		component_type_id_t const new_id = component_type_id_of<TC>();
		uint32_t const src_idx = entity_slots[id.index].archetype_index;
		uint32_t const src_chunk = entity_slots[id.index].chunk_index;
		uint32_t const src_row = entity_slots[id.index].row;

		// If the entity already has C, replace in place.
		{
			Archetype& src = archetypes[src_idx];
			std::size_t const existing_col = src.column_index_for(new_id);
			if (existing_col != NO_COLUMN_INDEX) {
				if constexpr (std::is_empty_v<TC>) {
					(void) value;
					return nullptr; // tag types have no data — no pointer to return.
				} else {
					TC* dst_ptr = static_cast<TC*>(src.row_in_column(src_chunk, existing_col, src_row));
					*dst_ptr = std::forward<C>(value);
					return dst_ptr;
				}
			}
		}

		// Build target signature = src.signature ∪ {new_id}, sorted ascending.
		std::vector<component_type_id_t> target_sig;
		std::vector<ColumnVTable const*> target_vtables;
		{
			Archetype const& src = archetypes[src_idx];
			target_sig.reserve(src.signature.size() + 1);
			target_vtables.reserve(src.signature.size() + 1);
			bool inserted = false;
			for (std::size_t i = 0; i < src.signature.size(); ++i) {
				component_type_id_t const sid = src.signature[i];
				if (!inserted && sid > new_id) {
					target_sig.push_back(new_id);
					target_vtables.push_back(&column_vtable_for<TC>());
					inserted = true;
				}
				target_sig.push_back(sid);
				target_vtables.push_back(src.vtables[i]);
			}
			if (!inserted) {
				target_sig.push_back(new_id);
				target_vtables.push_back(&column_vtable_for<TC>());
			}
		}

		// Look up / create the target archetype. NOTE: this may invalidate references into
		// `archetypes` if the vector grows. Re-resolve via index after this point.
		uint32_t const target_idx = find_or_create_archetype(target_sig, target_vtables.data());

		// Reserve a row on the target archetype.
		Archetype::RowLocation target_loc = archetypes[target_idx].reserve_row();
		archetypes[target_idx].entity_array(target_loc.chunk_index)[target_loc.row] = id;

		// Move every existing component from src to target, and construct the new one.
		C* placed_ptr = nullptr;
		{
			Archetype& target = archetypes[target_idx];
			Archetype& src = archetypes[src_idx];
			for (std::size_t i = 0; i < target.signature.size(); ++i) {
				component_type_id_t const tid = target.signature[i];
				if (tid == new_id) {
					if constexpr (!std::is_empty_v<TC>) {
						void* slot = target.row_in_column(target_loc.chunk_index, i, target_loc.row);
						placed_ptr = static_cast<C*>(slot);
						::new (slot) TC(std::forward<C>(value));
					} else {
						(void) value;
					}
				} else {
					std::size_t const src_col_idx = src.column_index_for(tid);
					if (target.column_offsets[i] != NO_COLUMN_OFFSET) {
						void* dst = target.row_in_column(target_loc.chunk_index, i, target_loc.row);
						void* srcp = src.row_in_column(src_chunk, src_col_idx, src_row);
						target.vtables[i]->move_construct(dst, srcp);
					}
					// Tag column: no data to move; archetype-level membership conveys it.
				}
			}
		}

		// Compact the source archetype — every src non-tag column has been moved-from at src_row.
		compact_archetype_after_external_move(src_idx, src_chunk, src_row);

		// Update slot to point at the new archetype/row.
		EntitySlot& slot = entity_slots[id.index];
		slot.archetype_index = target_idx;
		slot.chunk_index = static_cast<uint32_t>(target_loc.chunk_index);
		slot.row = static_cast<uint32_t>(target_loc.row);

		return placed_ptr;
	}

	template<typename C>
	C* World::add_component(EntityID id) {
		return add_component<C>(id, C {});
	}

	template<typename C>
	bool World::remove_component(EntityID id) {
		using TC = std::remove_cvref_t<C>;

		if (in_tick_or_log_("World::remove_component")) {
			return false;
		}
		if (!is_alive(id)) {
			return false;
		}
		if (immutable_or_log_(id, "World::remove_component", ComponentName<TC>::value)) {
			return false;
		}

		component_type_id_t const drop_id = component_type_id_of<TC>();
		uint32_t const src_idx = entity_slots[id.index].archetype_index;
		uint32_t const src_chunk = entity_slots[id.index].chunk_index;
		uint32_t const src_row = entity_slots[id.index].row;

		std::size_t drop_col_idx = NO_COLUMN_INDEX;
		{
			Archetype const& src = archetypes[src_idx];
			drop_col_idx = src.column_index_for(drop_id);
			if (drop_col_idx == NO_COLUMN_INDEX) {
				return false;
			}
			if (src.signature.size() == 1) {
				// Removing the only component would leave a zero-component entity, which
				// our model doesn't allow. Caller must destroy_entity in this case.
				return false;
			}
		}

		// Build target signature = src.signature ∖ {drop_id}.
		std::vector<component_type_id_t> target_sig;
		std::vector<ColumnVTable const*> target_vtables;
		{
			Archetype const& src = archetypes[src_idx];
			target_sig.reserve(src.signature.size() - 1);
			target_vtables.reserve(src.signature.size() - 1);
			for (std::size_t i = 0; i < src.signature.size(); ++i) {
				if (src.signature[i] == drop_id) {
					continue;
				}
				target_sig.push_back(src.signature[i]);
				target_vtables.push_back(src.vtables[i]);
			}
		}

		uint32_t const target_idx = find_or_create_archetype(target_sig, target_vtables.data());

		// Reserve a row in the target archetype.
		Archetype::RowLocation target_loc = archetypes[target_idx].reserve_row();
		archetypes[target_idx].entity_array(target_loc.chunk_index)[target_loc.row] = id;

		// Destroy the dropped component on the src side, move the rest to target.
		{
			Archetype& target = archetypes[target_idx];
			Archetype& src = archetypes[src_idx];
			if (src.column_offsets[drop_col_idx] != NO_COLUMN_OFFSET) {
				src.vtables[drop_col_idx]->destroy(src.row_in_column(src_chunk, drop_col_idx, src_row));
			}
			for (std::size_t i = 0; i < target.signature.size(); ++i) {
				component_type_id_t const tid = target.signature[i];
				std::size_t const src_col_idx = src.column_index_for(tid);
				if (target.column_offsets[i] == NO_COLUMN_OFFSET) {
					continue; // tag column
				}
				void* dst = target.row_in_column(target_loc.chunk_index, i, target_loc.row);
				void* srcp = src.row_in_column(src_chunk, src_col_idx, src_row);
				target.vtables[i]->move_construct(dst, srcp);
			}
		}

		// Compact src — every column at src_row is now moved-from / destroyed.
		compact_archetype_after_external_move(src_idx, src_chunk, src_row);

		EntitySlot& slot = entity_slots[id.index];
		slot.archetype_index = target_idx;
		slot.chunk_index = static_cast<uint32_t>(target_loc.chunk_index);
		slot.row = static_cast<uint32_t>(target_loc.row);

		return true;
	}

	namespace detail {
		// Helper for iteration: returns a C& for a given (archetype, chunk, row) — for tag /
		// empty types we return a reference to a static empty instance instead of dereferencing
		// the column's nullptr data pointer.
		template<typename C>
		C& deref_chunk_row(Archetype& arch, std::size_t col_idx, std::size_t chunk_idx, std::size_t row) {
			if constexpr (std::is_empty_v<C>) {
				static C instance {};
				(void) arch;
				(void) col_idx;
				(void) chunk_idx;
				(void) row;
				return instance;
			} else {
				return *static_cast<C*>(arch.row_in_column(chunk_idx, col_idx, row));
			}
		}

		// For Phase 3: pick out raw column array pointer for ChunkView. Returns a fixed
		// dummy pointer for tag types (callers should not dereference); for non-tag types
		// returns the slab base.
		template<typename C>
		C* chunk_array_for(Archetype& arch, std::size_t col_idx, std::size_t chunk_idx) {
			if constexpr (std::is_empty_v<C>) {
				(void) arch;
				(void) col_idx;
				(void) chunk_idx;
				return nullptr;
			} else {
				return static_cast<C*>(arch.column_array(chunk_idx, col_idx));
			}
		}

		// Returns C& at `row` from a previously-hoisted column-base pointer. For tag types
		// returns a reference to a static dummy (the hoisted pointer is nullptr and unused).
		// OV_RESTRICT on the parameter tells the compiler that, inside this call, `arr` does
		// not alias any other restrict-qualified pointer in scope — the assertion that the
		// per-row driver loops in for_each / for_each_with_entity / iterate_one_chunk_for_*
		// rely on to keep column bases in registers across the inner loop.
		template<typename C>
		C& row_ref_from_array(C* OV_RESTRICT arr, std::size_t row) {
			if constexpr (std::is_empty_v<C>) {
				static C instance {};
				(void) arr;
				(void) row;
				return instance;
			} else {
				return arr[row];
			}
		}
	}

	template<typename... Cs, typename Fn>
	void World::for_each(Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each requires at least one component");

		Query q;
		q.template with<Cs...>().build();
		for_each<Cs...>(q, std::forward<Fn>(fn));
	}

	template<typename... Cs, typename Fn>
	void World::for_each_with_entity(Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each_with_entity requires at least one component");

		Query q;
		q.template with<Cs...>().build();
		for_each_with_entity<Cs...>(q, std::forward<Fn>(fn));
	}

	template<typename... Cs, typename Fn>
	void World::for_each(Query const& query, Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each requires at least one component");

		QueryCacheKey key { query.require_ids, query.exclude_ids };
		std::vector<uint32_t> const& matched = resolve_query_cache(key).archetype_indices;

		for (uint32_t arch_idx : matched) {
			Archetype& arch = archetypes[arch_idx];
			std::size_t cols[sizeof...(Cs)];
			std::size_t i = 0;
			((cols[i++] = arch.column_index_for(component_type_id_of<Cs>())), ...);

			for (std::size_t chunk_idx = 0; chunk_idx < arch.chunks.size(); ++chunk_idx) {
				std::size_t const row_count = arch.chunks[chunk_idx].count;
				[&]<std::size_t... Is>(std::index_sequence<Is...>) {
					// Hoist typed column-base pointers — computed once per chunk. The row
					// loop indexes these directly; per-row pointer rederivation through
					// row_in_column is eliminated. row_ref_from_array's OV_RESTRICT param
					// carries the non-aliasing promise into the inlined body.
					auto arrs = std::tuple { detail::chunk_array_for<Cs>(
						arch, cols[Is], chunk_idx)... };
					for (std::size_t row = 0; row < row_count; ++row) {
						fn(detail::row_ref_from_array<Cs>(
							std::get<Is>(arrs), row)...);
					}
				}(std::index_sequence_for<Cs...> {});
			}
		}
	}

	template<typename... Cs, typename Fn>
	void World::for_each_with_entity(Query const& query, Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each_with_entity requires at least one component");

		QueryCacheKey key { query.require_ids, query.exclude_ids };
		std::vector<uint32_t> const& matched = resolve_query_cache(key).archetype_indices;

		for (uint32_t arch_idx : matched) {
			Archetype& arch = archetypes[arch_idx];
			std::size_t cols[sizeof...(Cs)];
			std::size_t i = 0;
			((cols[i++] = arch.column_index_for(component_type_id_of<Cs>())), ...);

			for (std::size_t chunk_idx = 0; chunk_idx < arch.chunks.size(); ++chunk_idx) {
				std::size_t const row_count = arch.chunks[chunk_idx].count;
				EntityID const* OV_RESTRICT eids = arch.entity_array(chunk_idx);
				[&]<std::size_t... Is>(std::index_sequence<Is...>) {
					auto arrs = std::tuple { detail::chunk_array_for<Cs>(
						arch, cols[Is], chunk_idx)... };
					for (std::size_t row = 0; row < row_count; ++row) {
						fn(eids[row], detail::row_ref_from_array<Cs>(
							std::get<Is>(arrs), row)...);
					}
				}(std::index_sequence_for<Cs...> {});
			}
		}
	}

	template<typename... Cs, typename Fn>
	void World::for_each_chunk(Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each_chunk requires at least one component");
		Query q;
		q.template with<Cs...>().build();
		for_each_chunk<Cs...>(q, std::forward<Fn>(fn));
	}

	template<typename... Cs, typename Fn>
	void World::for_each_chunk(Query const& query, Fn&& fn) {
		static_assert(sizeof...(Cs) > 0, "for_each_chunk requires at least one component");

		QueryCacheKey key { query.require_ids, query.exclude_ids };
		std::vector<uint32_t> const& matched = resolve_query_cache(key).archetype_indices;

		for (uint32_t arch_idx : matched) {
			Archetype& arch = archetypes[arch_idx];
			std::size_t cols[sizeof...(Cs)];
			std::size_t i = 0;
			((cols[i++] = arch.column_index_for(component_type_id_of<Cs>())), ...);

			for (std::size_t chunk_idx = 0; chunk_idx < arch.chunks.size(); ++chunk_idx) {
				std::size_t const row_count = arch.chunks[chunk_idx].count;
				if (row_count == 0) {
					continue;
				}
				[&]<std::size_t... Is>(std::index_sequence<Is...>) {
					ChunkView<Cs...> view {
						row_count,
						arch.entity_array(chunk_idx),
						{ detail::chunk_array_for<Cs>(arch, cols[Is], chunk_idx)... }
					};
					fn(view);
				}(std::index_sequence_for<Cs...> {});
			}
		}
	}

	template<typename C>
	C* World::set_singleton(C&& value) {
		using TC = std::remove_cvref_t<C>;
		component_type_id_t const id = component_type_id_of<TC>();
		auto deleter = +[](void* p) {
			delete static_cast<TC*>(p);
		};
		auto it = singletons.find(id);
		if (it != singletons.end()) {
			TC* existing = static_cast<TC*>(it->second.ptr.get());
			*existing = std::forward<C>(value);
			return existing;
		}
		TC* fresh = new TC(std::forward<C>(value));
		singletons.emplace(id, SingletonRecord {
			SingletonPtr { static_cast<void*>(fresh), deleter }, checksum_singleton_thunk_for<TC>()
		});
		return fresh;
	}

	template<typename C>
	C* World::set_singleton() {
		using TC = std::remove_cvref_t<C>;
		component_type_id_t const id = component_type_id_of<TC>();
		auto deleter = +[](void* p) {
			delete static_cast<TC*>(p);
		};
		auto it = singletons.find(id);
		if (it != singletons.end()) {
			TC* existing = static_cast<TC*>(it->second.ptr.get());
			*existing = TC {};
			return existing;
		}
		TC* fresh = new TC {};
		singletons.emplace(id, SingletonRecord {
			SingletonPtr { static_cast<void*>(fresh), deleter }, checksum_singleton_thunk_for<TC>()
		});
		return fresh;
	}

	template<typename C>
	C* World::get_singleton() {
		using TC = std::remove_cvref_t<C>;
		auto it = singletons.find(component_type_id_of<TC>());
		if (it == singletons.end()) {
			return nullptr;
		}
		return static_cast<TC*>(it->second.ptr.get());
	}

	template<typename C>
	C const* World::get_singleton() const {
		using TC = std::remove_cvref_t<C>;
		auto it = singletons.find(component_type_id_of<TC>());
		if (it == singletons.end()) {
			return nullptr;
		}
		return static_cast<TC const*>(it->second.ptr.get());
	}

	template<typename C>
	bool World::clear_singleton() {
		using TC = std::remove_cvref_t<C>;
		auto it = singletons.find(component_type_id_of<TC>());
		if (it == singletons.end()) {
			return false;
		}
		singletons.erase(it);
		return true;
	}

	// === Templated register_system<S, Args...>(...) ===

	template<typename S, typename... Args>
	SystemHandle World::register_system(Args&&... args) {
		static_assert(
			!(S::is_threaded && S::extra_writes().size() > 0),
			"SystemThreaded must not declare extra_writes(): its chunks run concurrently, so a "
			"cross-archetype/singleton write races the system with ITSELF — no scheduler edge "
			"can fix that. Use a serial System<> (with Reductions::* for parallel folds) instead."
		);

		static_assert(
			!has_should_run_v<S> || should_run_signature_valid_v<S>,
			"S declares `should_run`, but not as `static bool should_run(TickContext const&)`. "
			"It must be a single, non-overloaded STATIC function with exactly that signature "
			"(noexcept allowed) — systems are stateless by project rule. A member function, "
			"data member, overload set, or wrong-signature variant would otherwise be silently "
			"ignored; this assert makes it a hard error instead."
		);

		// Build a SystemRegistration from S's static metadata. The system's per-row tick
		// is non-virtual; we store a thunk (`tick_all_fn`) that resolves to
		// `static_cast<S*>(instance)->tick_all(world, ctx)` — a single virtual-ish call
		// outside the hot iteration loop, kept type-erased so the registry can hold
		// heterogeneous system types in one vector.

		uint32_t const idx = static_cast<uint32_t>(system_registry_.size());

		auto instance_owned = std::make_unique<S>(std::forward<Args>(args)...);
		void* instance_raw = instance_owned.get();
		auto deleter = +[](void* p) { delete static_cast<S*>(p); };
		auto tick_all_fn = +[](void* inst, World& w, TickContext const& tc) {
			static_cast<S*>(inst)->tick_all(w, tc);
		};

		// Extract static metadata from S. Each `declared_*` returns a std::array; we copy
		// into the registration's owned vectors so the spans / refs into them stay valid
		// across registry growth.
		constexpr auto access_arr = S::declared_access();
		constexpr auto run_after_arr = S::declared_run_after();
		constexpr auto run_before_arr = S::declared_run_before();
		constexpr auto extra_reads_arr = S::extra_reads();
		constexpr auto extra_writes_arr = S::extra_writes();

		SystemRegistration reg;
		reg.instance = instance_raw;
		reg.deleter = deleter;
		reg.tick_all_fn = tick_all_fn;
		reg.type_id = system_type_id_of<S>();
		reg.name = SystemName<S>::value;
		reg.is_threaded = S::is_threaded;
		reg.access.assign(access_arr.begin(), access_arr.end());
		canonicalise_access_set(reg.access);
		reg.run_after.assign(run_after_arr.begin(), run_after_arr.end());
		reg.run_before.assign(run_before_arr.begin(), run_before_arr.end());
		reg.extra_reads.assign(extra_reads_arr.begin(), extra_reads_arr.end());
		reg.extra_writes.assign(extra_writes_arr.begin(), extra_writes_arr.end());
		// Sorted-unique so provably_disjoint can binary_search them (same convention as
		// tick_query_require_ids). Declaration order is not meaningful — these only ever
		// fold into the access set and gate the disjoint-iteration override.
		std::sort(reg.extra_reads.begin(), reg.extra_reads.end());
		reg.extra_reads.erase(
			std::unique(reg.extra_reads.begin(), reg.extra_reads.end()), reg.extra_reads.end()
		);
		std::sort(reg.extra_writes.begin(), reg.extra_writes.end());
		reg.extra_writes.erase(
			std::unique(reg.extra_writes.begin(), reg.extra_writes.end()), reg.extra_writes.end()
		);
		merge_extra_reads(reg.access, reg.extra_reads);
		merge_extra_writes(reg.access, reg.extra_writes);
		canonicalise_access_set(reg.access);

		// Iteration-query ids — tick parameter pack only, separate from `access` (which
		// includes extra_reads after merge). Used by the scheduler to prewarm query_cache
		// before a multi-system stage's outer parallel_for. Plain System<> systems use it
		// for the same reason — their `for_each` inside dispatch_serial would otherwise
		// race on query_cache mutation when running concurrently with another System<>.
		reg.tick_query_require_ids = S::compute_tick_query_require_ids();
		// Exclude ids from the system's optional `Filters` alias (empty for unfiltered systems).
		// Stored alongside require so the scheduler prewarms the exact key every dispatch path
		// later looks up — see detail::build_tick_query.
		reg.tick_query_exclude_ids = S::compute_tick_query_exclude_ids();

		// Multi-system-stage entry points — set only for SystemThreaded. Plain System<>
		// stays with null pointers; the scheduler distinguishes via reg.is_threaded.
		if constexpr (S::is_threaded) {
			reg.collect_chunks_fn = +[](World& w) -> std::vector<ChunkLocation> {
				return S::collect_chunks(w);
			};
			reg.tick_one_chunk_fn = +[](
				void* inst, World& w, TickContext const& tc,
				uint32_t arch_idx, uint32_t chunk_idx
			) {
				S::tick_one_chunk(*static_cast<S*>(inst), w, tc, arch_idx, chunk_idx);
			};
			reg.per_chunk_cmds_accessor = +[](void* inst) -> std::vector<CommandBuffer>* {
				return &static_cast<S*>(inst)->per_chunk_buffers();
			};
		}

		// Optional per-tick gate. The thunk shape matches tick_all_fn et al; gated on
		// VALIDITY (not presence) so a failed should_run static_assert above does not
		// cascade into a second error from forming the call here.
		if constexpr (should_run_signature_valid_v<S>) {
			reg.should_run_fn = +[](TickContext const& tc) -> bool {
				return S::should_run(tc);
			};
		}

		reg.alive = true;
		reg.generation = 1;

		// Allocate this system's pending CommandBuffer (heap-stable across registry vector
		// growth). Out-of-line: keeps the incomplete-type CommandBuffer out of this template.
		reg.pending_cmd = allocate_pending_cmd_();

		// Move the instance ownership into the registration last, so partial-failure paths
		// don't leak. (`instance_raw` was already captured.)
		(void) instance_owned.release();
		system_registry_.push_back(std::move(reg));

		scheduler_dirty_ = true;
		return SystemHandle { idx, 1 };
	}

	// === Per-chunk iteration for SystemThreaded ===

	template<typename... Cs, typename Body>
	void World::iterate_one_chunk_for_threaded(uint32_t archetype_idx, uint32_t chunk_idx, Body&& body) {
		Archetype& arch = archetypes[archetype_idx];
		std::size_t cols[sizeof...(Cs)];
		std::size_t i = 0;
		((cols[i++] = arch.column_index_for(component_type_id_of<Cs>())), ...);
		std::size_t const row_count = arch.chunks[chunk_idx].count;
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			auto arrs = std::tuple { detail::chunk_array_for<Cs>(
				arch, cols[Is], chunk_idx)... };
			for (std::size_t row = 0; row < row_count; ++row) {
				body(detail::row_ref_from_array<Cs>(
					std::get<Is>(arrs), row)...);
			}
		}(std::index_sequence_for<Cs...> {});
	}

	template<typename... Cs, typename Body>
	void World::iterate_one_chunk_with_entity_for_threaded(uint32_t archetype_idx, uint32_t chunk_idx, Body&& body) {
		Archetype& arch = archetypes[archetype_idx];
		std::size_t cols[sizeof...(Cs)];
		std::size_t i = 0;
		((cols[i++] = arch.column_index_for(component_type_id_of<Cs>())), ...);
		std::size_t const row_count = arch.chunks[chunk_idx].count;
		EntityID const* OV_RESTRICT eids = arch.entity_array(chunk_idx);
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			auto arrs = std::tuple { detail::chunk_array_for<Cs>(
				arch, cols[Is], chunk_idx)... };
			for (std::size_t row = 0; row < row_count; ++row) {
				body(eids[row], detail::row_ref_from_array<Cs>(
					std::get<Is>(arrs), row)...);
			}
		}(std::index_sequence_for<Cs...> {});
	}
}
