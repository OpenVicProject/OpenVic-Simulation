#pragma once

// Header that closes the System.hpp ↔ World.hpp dependency cycle by including both
// World.hpp (full templates) and System.hpp (CRTP base + driver declarations) and then
// defining the iteration drivers `dispatch_serial` and `dispatch_threaded`.
//
// Every concrete system header (UnitGroupTotalsSystem.hpp etc.) should include this
// header rather than System.hpp directly, so that the templated `tick_all` methods on
// the CRTP base can be instantiated correctly.

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic::ecs::detail {

	// Tuple-unpacking helper for serial dispatch.
	template<typename Derived, bool WithEntity, typename Tuple>
	struct dispatch_serial_impl;

	template<typename Derived, typename... Cs>
	struct dispatch_serial_impl<Derived, /*WithEntity=*/false, std::tuple<Cs...>> {
		static void run(Derived& self, World& world, TickContext const& ctx) {
			static_assert(sizeof...(Cs) > 0,
				"System::tick must have at least one component parameter after TickContext");
			world.template for_each<std::remove_cvref_t<Cs>...>(
				[&self, &ctx](std::remove_cvref_t<Cs>&... cs) {
					self.tick(ctx, cs...);
				});
		}
	};

	template<typename Derived, typename... Cs>
	struct dispatch_serial_impl<Derived, /*WithEntity=*/true, std::tuple<Cs...>> {
		static void run(Derived& self, World& world, TickContext const& ctx) {
			world.template for_each_with_entity<std::remove_cvref_t<Cs>...>(
				[&self, &ctx](EntityID eid, std::remove_cvref_t<Cs>&... cs) {
					self.tick(ctx, eid, cs...);
				});
		}
	};

	template<typename Derived>
	void dispatch_serial(Derived& self, World& world, TickContext const& ctx) {
		using Components = component_pack_t<Derived>;
		constexpr bool with_entity = tick_takes_entity_v<Derived>;
		dispatch_serial_impl<Derived, with_entity, Components>::run(self, world, ctx);
	}

	// Threaded dispatch — parallelises across chunks using the EcsThreadPool. Per-chunk
	// CommandBuffers are stored in `per_chunk_cmds`, indexed by chunk_idx. After the
	// parallel section joins, buffers are merged into `pending_cmd` in chunk_idx
	// ascending order — making the apply sequence identical regardless of worker count.

	template<typename Derived, bool WithEntity, typename Tuple>
	struct dispatch_threaded_impl;

	// Per-chunk-iteration helpers reach into World's internals for the chunk-by-chunk
	// raw view; serial path uses `world.for_each<...>` which already does this internally.
	// For the threaded path we need a flat list of (archetype_idx, chunk_idx) pairs so we
	// can dispatch them across workers.
	//
	// `ChunkLocation` lives at namespace scope in System.hpp (so SystemRegistration's
	// function-pointer signatures can name it) — this file just uses it.

	// Collect all chunks matching the query of components Cs.... Sorted by
	// (archetype_idx ascending, chunk_idx ascending) — deterministic.
	template<typename... Cs>
	std::vector<ChunkLocation> collect_matching_chunks(World& world) {
		Query q;
		q.template with<std::remove_cvref_t<Cs>...>().build();
		QueryCacheKey key { q.require_ids, q.exclude_ids };
		std::vector<uint32_t> const& matched
			= world.resolve_query_cache_for_threaded(key).archetype_indices;

		std::vector<ChunkLocation> out;
		for (uint32_t arch_idx : matched) {
			std::size_t const chunk_count = world.archetype_chunk_count(arch_idx);
			for (std::size_t c = 0; c < chunk_count; ++c) {
				if (world.archetype_chunk_row_count(arch_idx, c) > 0) {
					out.push_back(ChunkLocation { arch_idx, static_cast<uint32_t>(c) });
				}
			}
		}
		return out;
	}

	template<typename Derived, typename... Cs>
	struct dispatch_threaded_impl<Derived, /*WithEntity=*/false, std::tuple<Cs...>> {
		static void run(
			Derived& self, World& world, TickContext const& ctx_template,
			EcsThreadPool& pool, std::vector<CommandBuffer>& per_chunk_cmds,
			CommandBuffer& pending_cmd
		) {
			std::vector<ChunkLocation> const chunks
				= collect_matching_chunks<std::remove_cvref_t<Cs>...>(world);
			std::size_t const N = chunks.size();
			if (N == 0) {
				return;
			}

			// Pool: resize per-chunk CB vector and clear each.
			if (per_chunk_cmds.size() < N) {
				per_chunk_cmds.resize(N);
			}
			for (std::size_t i = 0; i < N; ++i) {
				per_chunk_cmds[i].clear();
				per_chunk_cmds[i].set_parallel_mode(true);
			}

			pool.parallel_for(N, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
				ChunkLocation const& loc = chunks[chunk_idx];
				CommandBuffer& cmd = per_chunk_cmds[chunk_idx];
				TickContext per_chunk { ctx_template.world, ctx_template.today, cmd };
				world.template iterate_one_chunk_for_threaded<std::remove_cvref_t<Cs>...>(
					loc.archetype_idx, loc.chunk_idx,
					[&self, &per_chunk](std::remove_cvref_t<Cs>&... cs) {
						self.tick(per_chunk, cs...);
					});
			});

			// Merge in chunk_idx ascending order — deterministic regardless of worker_count.
			for (std::size_t i = 0; i < N; ++i) {
				per_chunk_cmds[i].set_parallel_mode(false);
				pending_cmd.merge_from(std::move(per_chunk_cmds[i]));
			}
		}
	};

	template<typename Derived, typename... Cs>
	struct dispatch_threaded_impl<Derived, /*WithEntity=*/true, std::tuple<Cs...>> {
		static void run(
			Derived& self, World& world, TickContext const& ctx_template,
			EcsThreadPool& pool, std::vector<CommandBuffer>& per_chunk_cmds,
			CommandBuffer& pending_cmd
		) {
			std::vector<ChunkLocation> const chunks
				= collect_matching_chunks<std::remove_cvref_t<Cs>...>(world);
			std::size_t const N = chunks.size();
			if (N == 0) {
				return;
			}

			if (per_chunk_cmds.size() < N) {
				per_chunk_cmds.resize(N);
			}
			for (std::size_t i = 0; i < N; ++i) {
				per_chunk_cmds[i].clear();
				per_chunk_cmds[i].set_parallel_mode(true);
			}

			pool.parallel_for(N, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
				ChunkLocation const& loc = chunks[chunk_idx];
				CommandBuffer& cmd = per_chunk_cmds[chunk_idx];
				TickContext per_chunk { ctx_template.world, ctx_template.today, cmd };
				world.template iterate_one_chunk_with_entity_for_threaded<
					std::remove_cvref_t<Cs>...>(
					loc.archetype_idx, loc.chunk_idx,
					[&self, &per_chunk](EntityID eid, std::remove_cvref_t<Cs>&... cs) {
						self.tick(per_chunk, eid, cs...);
					});
			});

			for (std::size_t i = 0; i < N; ++i) {
				per_chunk_cmds[i].set_parallel_mode(false);
				pending_cmd.merge_from(std::move(per_chunk_cmds[i]));
			}
		}
	};

	template<typename Derived>
	void dispatch_threaded(
		Derived& self, World& world, TickContext const& ctx,
		EcsThreadPool& pool, std::vector<CommandBuffer>& per_chunk_cmds,
		CommandBuffer& pending_cmd
	) {
		using Components = component_pack_t<Derived>;
		constexpr bool with_entity = tick_takes_entity_v<Derived>;
		dispatch_threaded_impl<Derived, with_entity, Components>::run(
			self, world, ctx, pool, per_chunk_cmds, pending_cmd);
	}
}

namespace OpenVic::ecs {
	// Out-of-line definition of `SystemThreaded<Derived>::tick_all`. Defined here so the
	// template body has visibility into World, CommandBuffer, and EcsThreadPool.
	//
	// Used by single-system stages — the scheduler sets current_system_registration_
	// before calling tick_all_fn. Multi-system stages bypass this entirely, calling
	// collect_chunks + tick_one_chunk per chunk inside one outer parallel_for.
	template<typename Derived>
	void SystemThreaded<Derived>::tick_all(World& world, TickContext const& ctx) {
		// Find the SystemRegistration for this instance to get pending_cmd + thread pool.
		// The scheduler installs a thread-local pointer to the registration before invoking
		// tick_all, so we retrieve the per-instance pending_cmd from there.
		SystemRegistration* reg = world.current_system_registration();
		if (reg == nullptr || reg->pending_cmd == nullptr) {
			// Fallback: serial — should not happen in normal operation, but keeps the
			// system functional during boot or test scenarios where no scheduler is installed.
			detail::dispatch_serial<Derived>(static_cast<Derived&>(*this), world, ctx);
			return;
		}
		EcsThreadPool& pool = world.ecs_thread_pool();
		detail::dispatch_threaded<Derived>(
			static_cast<Derived&>(*this), world, ctx, pool, per_chunk_cmds_, *reg->pending_cmd);
	}

	// === Multi-system-stage entry points ===
	//
	// These are invoked by SystemScheduler when this SystemThreaded shares a stage with
	// one or more other systems. `collect_chunks` runs on the main thread before dispatch;
	// `tick_one_chunk` is invoked per chunk by the outer parallel_for. The combined work-
	// item list across every system in the stage prevents nested parallel_for (which would
	// deadlock workers blocked in cv.wait while the inner jobs sit unowned in the queue).

	template<typename Derived>
	std::vector<ChunkLocation> SystemThreaded<Derived>::collect_chunks(World& world) {
		return [&]<typename... Cs>(std::tuple<Cs...>*) {
			return detail::collect_matching_chunks<std::remove_cvref_t<Cs>...>(world);
		}(static_cast<detail::component_pack_t<Derived>*>(nullptr));
	}

	template<typename Derived>
	void SystemThreaded<Derived>::tick_one_chunk(
		Derived& self, World& world, TickContext const& ctx,
		uint32_t archetype_idx, uint32_t chunk_idx
	) {
		using Components = detail::component_pack_t<Derived>;
		constexpr bool with_entity = detail::tick_takes_entity_v<Derived>;

		[&]<typename... Cs>(std::tuple<Cs...>*) {
			if constexpr (with_entity) {
				world.template iterate_one_chunk_with_entity_for_threaded<
					std::remove_cvref_t<Cs>...>(
					archetype_idx, chunk_idx,
					[&self, &ctx](EntityID eid, std::remove_cvref_t<Cs>&... cs) {
						self.tick(ctx, eid, cs...);
					}
				);
			} else {
				world.template iterate_one_chunk_for_threaded<std::remove_cvref_t<Cs>...>(
					archetype_idx, chunk_idx,
					[&self, &ctx](std::remove_cvref_t<Cs>&... cs) {
						self.tick(ctx, cs...);
					}
				);
			}
		}(static_cast<Components*>(nullptr));
	}
}
