#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemAccess.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic::ecs {
	struct World;
	struct CommandBuffer;
	class EcsThreadPool;

	// (archetype_idx, chunk_idx) pair identifying one chunk of a matched archetype. Used
	// by the scheduler's multi-system parallel branch to build a flat work-item list across
	// every SystemThreaded in a stage, dispatched via one outer parallel_for. Lives at
	// namespace scope (not detail) so SystemRegistration can store a function pointer
	// returning std::vector<ChunkLocation> without circular includes.
	struct ChunkLocation {
		uint32_t archetype_idx;
		uint32_t chunk_idx;
	};

	// Context passed to every System on each tick. Carries whatever a System might want to
	// know about the simulation state at the moment of the tick, plus the system's
	// CommandBuffer for deferred structural mutations. For SystemThreaded, the `cmd`
	// reference points at the per-chunk CommandBuffer the driver allocates for the row's
	// chunk.
	struct TickContext {
		World& world;
		Date today;
		CommandBuffer& cmd;
	};

	// Stable handle returned by `register_system`. Generation is bumped on `unregister_system`
	// so a stale handle reliably fails an `is_valid` / `unregister_system` check rather than
	// silently mutating the wrong slot.
	struct SystemHandle {
		uint32_t index = 0;
		uint32_t generation = 0;

		constexpr bool operator==(SystemHandle const& rhs) const {
			return index == rhs.index && generation == rhs.generation;
		}
		constexpr bool operator!=(SystemHandle const& rhs) const {
			return !(*this == rhs);
		}
		// Generation 0 is the invalid sentinel — valid handles always have generation >= 1.
		constexpr bool is_valid() const {
			return generation != 0;
		}
	};

	inline constexpr SystemHandle INVALID_SYSTEM_HANDLE = {};

	// === Member-function-trait machinery for extracting the component pack from
	//     `&Derived::tick`. ===

	namespace detail {
		// Type-based member-function-traits. Use as `fn_traits<decltype(&Derived::tick)>`.
		// MSVC doesn't accept `auto`-non-type template params for member-function pointers
		// with variadic Args, so we go through the function-pointer type instead.
		template<typename MemberFnPtr>
		struct fn_traits;

		template<typename C, typename R, typename... Args>
		struct fn_traits<R (C::*)(Args...)> {
			using class_type = C;
			using return_type = R;
			using args_tuple = std::tuple<Args...>;
			static constexpr std::size_t arg_count = sizeof...(Args);
		};

		template<typename C, typename R, typename... Args>
		struct fn_traits<R (C::*)(Args...) const> {
			using class_type = C;
			using return_type = R;
			using args_tuple = std::tuple<Args...>;
			static constexpr std::size_t arg_count = sizeof...(Args);
		};

		// Strip leading `TickContext const&` (and optional `EntityID`) from the args tuple,
		// yielding the component pack.
		template<typename Tuple>
		struct strip_context_and_entity;

		template<typename... Cs>
		struct strip_context_and_entity<std::tuple<TickContext const&, EntityID, Cs...>> {
			using components = std::tuple<Cs...>;
			static constexpr bool takes_entity = true;
			static constexpr bool takes_immutable_entity = false;
		};

		// A tick may instead take an ImmutableEntityID as its per-row handle: the system iterates
		// the same rows, but the handle it receives cannot be passed to add_component /
		// remove_component (compile-time guarantee inside the tick body). Uses the same with-entity
		// iteration path; the driver wraps the yielded EntityID (see SystemImpl.hpp).
		template<typename... Cs>
		struct strip_context_and_entity<std::tuple<TickContext const&, ImmutableEntityID, Cs...>> {
			using components = std::tuple<Cs...>;
			static constexpr bool takes_entity = true;
			static constexpr bool takes_immutable_entity = true;
		};

		template<typename... Cs>
		struct strip_context_and_entity<std::tuple<TickContext const&, Cs...>> {
			using components = std::tuple<Cs...>;
			static constexpr bool takes_entity = false;
			static constexpr bool takes_immutable_entity = false;
		};

		template<typename Derived>
		using tick_args_tuple_t =
			typename fn_traits<decltype(&Derived::tick)>::args_tuple;

		template<typename Derived>
		using component_pack_t =
			typename strip_context_and_entity<tick_args_tuple_t<Derived>>::components;

		template<typename Derived>
		constexpr bool tick_takes_entity_v =
			strip_context_and_entity<tick_args_tuple_t<Derived>>::takes_entity;

		// True when Derived::tick takes an ImmutableEntityID (rather than EntityID) as its per-row
		// handle. The iteration path is the same with-entity path; only the handle type the driver
		// hands to tick() differs — see the dispatch lambdas in SystemImpl.hpp.
		template<typename Derived>
		constexpr bool tick_takes_immutable_entity_v =
			strip_context_and_entity<tick_args_tuple_t<Derived>>::takes_immutable_entity;

		// Build a static AccessSet array from a component pack tuple.
		template<typename Tuple>
		struct access_set_from_tuple;

		template<typename... Cs>
		struct access_set_from_tuple<std::tuple<Cs...>> {
			static constexpr std::size_t N = sizeof...(Cs);
			static constexpr std::array<ComponentAccess, N> value() {
				return { ComponentAccess {
					component_type_id_of<std::remove_cvref_t<Cs>>(),
					std::is_const_v<std::remove_reference_t<Cs>> ? AccessMode::Read : AccessMode::Write
				}... };
			}
		};

		template<typename Derived>
		constexpr auto compute_access_set() {
			return access_set_from_tuple<component_pack_t<Derived>>::value();
		}

		// Build a sorted-unique vector of component_type_id_t from a tick parameter pack.
		// Used by the scheduler's query-cache prewarm pass — captures the iteration query
		// (tick params only, NOT extra_reads) so the scheduler can populate query_cache on
		// the main thread before dispatching workers.
		template<typename Tuple>
		struct require_ids_from_tuple;

		template<typename... Cs>
		struct require_ids_from_tuple<std::tuple<Cs...>> {
			static std::vector<component_type_id_t> compute() {
				std::vector<component_type_id_t> ids = {
					component_type_id_of<std::remove_cvref_t<Cs>>()...
				};
				std::sort(ids.begin(), ids.end());
				ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
				return ids;
			}
		};

		// Iteration drivers — declared here, defined in SystemImpl.hpp (which transitively
		// includes World.hpp + CommandBuffer.hpp + EcsThreadPool.hpp). Templates so the
		// definitions are picked up at instantiation, breaking the include cycle.
		template<typename Derived>
		void dispatch_serial(Derived& self, World& world, TickContext const& ctx);

		template<typename Derived>
		void dispatch_threaded(
			Derived& self, World& world, TickContext const& ctx,
			EcsThreadPool& pool, std::vector<CommandBuffer>& per_chunk_cmds,
			CommandBuffer& pending_cmd
		);
	}

	// Invokes Derived::tick for one row that the with-entity iteration path produced. The handle
	// type is chosen at compile time from the tick signature: if the tick declared an
	// ImmutableEntityID parameter, the yielded EntityID is wrapped into one (so add_component /
	// remove_component on it won't compile inside the tick body); otherwise the EntityID is passed
	// straight through. Lives in OpenVic::ecs (not detail) so both the detail dispatch impls and
	// the OpenVic::ecs tick_one_chunk can call it unqualified. Zero cost — the wrap is an 8-byte
	// copy the optimiser folds away, and the non-immutable branch is identical to a direct call.
	template<typename Derived, typename... Cs>
	inline void invoke_tick_with_entity(Derived& self, TickContext const& ctx, EntityID eid, Cs&... cs) {
		if constexpr (detail::tick_takes_immutable_entity_v<Derived>) {
			self.tick(ctx, ImmutableEntityID { eid.index, eid.generation }, cs...);
		} else {
			self.tick(ctx, eid, cs...);
		}
	}

	// === System base — CRTP, non-virtual tick. ===

	template<typename Derived>
	struct System {
		// Derived must implement (non-virtual!):
		//   void tick(TickContext const& ctx, [EntityID,] Cs&... components);
		// where Cs... carry access intent: `C const` = Read, `C` = Write.

		static constexpr bool is_threaded = false;

		// Compile-time access set, derived from &Derived::tick's signature.
		static constexpr auto declared_access() {
			return detail::compute_access_set<Derived>();
		}

		// Returns the system_type_id_t for Derived. Derived must have an ECS_SYSTEM(Type)
		// declaration at namespace scope.
		static constexpr system_type_id_t type_id() {
			return system_type_id_of<Derived>();
		}

		// Default empty dependency lists. Override on the derived class with
		//   static constexpr std::array<system_type_id_t, N> declared_run_after();
		// (or `_before` / `extra_reads`) to add explicit ordering or cross-archetype reads.
		static constexpr std::array<system_type_id_t, 0> declared_run_after() { return {}; }
		static constexpr std::array<system_type_id_t, 0> declared_run_before() { return {}; }
		static constexpr std::array<component_type_id_t, 0> extra_reads() { return {}; }

		// Sorted-unique component ids that define this system's iteration query — the tick
		// parameter pack only, NOT extra_reads. Read by the scheduler at registration time
		// and again per-tick to prewarm the World's query cache before a multi-system stage
		// dispatches workers, so resolve_query_cache never has to mutate its hashmap from a
		// worker thread.
		static std::vector<component_type_id_t> compute_tick_query_require_ids() {
			return detail::require_ids_from_tuple<detail::component_pack_t<Derived>>::compute();
		}

		// Drives serial iteration. Called once per tick by the scheduler. Defined inline
		// because dispatch_serial is a template — instantiated at the point a derived
		// system's tick_all_fn is taken (which requires SystemImpl.hpp visible).
		void tick_all(World& world, TickContext const& ctx) {
			detail::dispatch_serial<Derived>(static_cast<Derived&>(*this), world, ctx);
		}
	};

	template<typename Derived>
	struct SystemThreaded : System<Derived> {
		static constexpr bool is_threaded = true;

		// Drives chunk-parallel iteration via the World's EcsThreadPool. Per-chunk
		// CommandBuffers (indexed by chunk_idx) are allocated, populated, then merged into
		// the system's pending CommandBuffer in chunk_idx ascending order — making the
		// apply order deterministic and identical across all worker counts.
		//
		// Used by the scheduler ONLY for single-system stages. For multi-system stages the
		// scheduler instead invokes collect_chunks + tick_one_chunk per chunk inside one
		// outer parallel_for that mixes work from every system in the stage — avoiding
		// nested parallel_for and the current_system_registration_ race.
		void tick_all(World& world, TickContext const& ctx);

		// Multi-system-stage entry points. The scheduler calls collect_chunks once on the
		// main thread to enumerate matched chunks (in (arch_idx, chunk_idx) ascending order),
		// then dispatches one work item per chunk via the outer parallel_for. Each work item
		// invokes tick_one_chunk with that chunk's per_chunk_cmds_ slot as TickContext::cmd.
		static std::vector<ChunkLocation> collect_chunks(World& world);
		static void tick_one_chunk(
			Derived& self, World& world, TickContext const& ctx,
			uint32_t archetype_idx, uint32_t chunk_idx
		);

		// Pooled across ticks to avoid per-tick allocator churn.
		std::vector<CommandBuffer>& per_chunk_buffers() { return per_chunk_cmds_; }

	private:
		std::vector<CommandBuffer> per_chunk_cmds_;
	};

	// === Type-erased per-system metadata stored by the scheduler. ===

	struct SystemRegistration {
		// Owned instance (type-erased via deleter).
		void* instance = nullptr;
		void (*deleter)(void*) = nullptr;
		// One-call-per-tick entry that resolves to `static_cast<S*>(instance)->tick_all(...)`.
		// Used by single-system stages, and by plain-System<> work items in multi-system stages.
		void (*tick_all_fn)(void* /*instance*/, World&, TickContext const&) = nullptr;

		// Multi-system-stage entry points. Set only for SystemThreaded (is_threaded == true);
		// null on plain System<>. The scheduler uses these to drive one outer parallel_for
		// over a combined work-item list across every system in a multi-system stage.
		std::vector<ChunkLocation> (*collect_chunks_fn)(World& world) = nullptr;
		void (*tick_one_chunk_fn)(
			void* /*instance*/, World&, TickContext const&,
			uint32_t /*archetype_idx*/, uint32_t /*chunk_idx*/
		) = nullptr;
		// Returns the system's per-chunk CommandBuffer pool. Scheduler resizes / clears /
		// flips parallel_mode through this accessor when building work items and again on
		// the merge pass after join.
		std::vector<CommandBuffer>* (*per_chunk_cmds_accessor)(void* /*instance*/) = nullptr;

		system_type_id_t type_id = 0;
		std::string_view name;
		bool is_threaded = false;

		// Owned copies of the static metadata extracted from the derived class at
		// registration time. Owned (not span'd into static storage) because the derived's
		// `static constexpr` returns by value — we need stable storage.
		std::vector<ComponentAccess> access;
		std::vector<system_type_id_t> run_after;
		std::vector<system_type_id_t> run_before;
		std::vector<component_type_id_t> extra_reads;

		// Sorted-unique component ids that define the iteration query — tick parameter
		// pack only, NOT extra_reads. The scheduler walks every system in a multi-system
		// stage and prewarms World::query_cache with these keys on the main thread before
		// dispatching workers, eliminating the latent query_cache race that plain System<>s
		// previously had in the run_concurrent path.
		std::vector<component_type_id_t> tick_query_require_ids;

		// Pending command buffer for this system this tick. Drained by the scheduler at
		// the stage barrier in registration_index ascending order across the stage.
		CommandBuffer* pending_cmd = nullptr;

		// Generation for SystemHandle validation; bumped on unregister.
		uint32_t generation = 1;
		bool alive = false;
	};
}
