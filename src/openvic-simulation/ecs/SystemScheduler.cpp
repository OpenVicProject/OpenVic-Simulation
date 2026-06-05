#include "openvic-simulation/ecs/SystemScheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/SystemAccess.hpp"
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;

namespace {
	// Adjacency list representation indexed by registration index (NOT system_type_id_t,
	// since registry_ may have unregistered/dead slots).
	struct DAG {
		std::size_t n = 0;
		std::vector<std::vector<uint32_t>> out_edges; // out_edges[u] = {v: u→v}
		std::vector<std::vector<uint32_t>> in_edges;
		std::vector<uint32_t> depth;

		void resize(std::size_t count) {
			n = count;
			out_edges.assign(count, {});
			in_edges.assign(count, {});
			depth.assign(count, 0);
		}

		void add_edge(uint32_t u, uint32_t v) {
			// Skip duplicates.
			for (uint32_t existing : out_edges[u]) {
				if (existing == v) {
					return;
				}
			}
			out_edges[u].push_back(v);
			in_edges[v].push_back(u);
		}

		// Returns true if there is a directed path from `from` to `to` (transitive).
		bool path_exists(uint32_t from, uint32_t to) const {
			if (from == to) {
				return true;
			}
			std::vector<bool> visited(n, false);
			std::vector<uint32_t> stack;
			stack.push_back(from);
			while (!stack.empty()) {
				uint32_t u = stack.back();
				stack.pop_back();
				if (u == to) {
					return true;
				}
				if (visited[u]) {
					continue;
				}
				visited[u] = true;
				for (uint32_t v : out_edges[u]) {
					if (!visited[v]) {
						stack.push_back(v);
					}
				}
			}
			return false;
		}

		// Recompute depths after edges have been added. Topological propagation:
		// depth[v] = max over preds u of (depth[u] + 1).
		bool recompute_depths_or_detect_cycle() {
			std::vector<uint32_t> indeg(n, 0);
			for (std::size_t v = 0; v < n; ++v) {
				indeg[v] = static_cast<uint32_t>(in_edges[v].size());
			}
			std::queue<uint32_t> q;
			depth.assign(n, 0);
			for (std::size_t v = 0; v < n; ++v) {
				if (indeg[v] == 0) {
					q.push(static_cast<uint32_t>(v));
				}
			}
			std::size_t processed = 0;
			while (!q.empty()) {
				uint32_t u = q.front();
				q.pop();
				++processed;
				for (uint32_t v : out_edges[u]) {
					uint32_t const cand = depth[u] + 1;
					if (cand > depth[v]) {
						depth[v] = cand;
					}
					if (--indeg[v] == 0) {
						q.push(v);
					}
				}
			}
			return processed == n; // false = cycle
		}
	};
}

void SystemScheduler::rebuild(std::vector<SystemRegistration>& registry) {
	std::size_t const N = registry.size();
	DAG dag;
	dag.resize(N);

	// Map: system_type_id_t -> registration index for every alive registration.
	std::unordered_map<system_type_id_t, uint32_t> type_to_idx;
	for (uint32_t i = 0; i < N; ++i) {
		if (registry[i].alive) {
			type_to_idx.emplace(registry[i].type_id, i);
		}
	}

	// Phase 1: declared edges (run_after / run_before).
	for (uint32_t i = 0; i < N; ++i) {
		if (!registry[i].alive) {
			continue;
		}
		for (system_type_id_t pred_id : registry[i].run_after) {
			auto it = type_to_idx.find(pred_id);
			if (it != type_to_idx.end()) {
				dag.add_edge(it->second, i);
			}
		}
		for (system_type_id_t succ_id : registry[i].run_before) {
			auto it = type_to_idx.find(succ_id);
			if (it != type_to_idx.end()) {
				dag.add_edge(i, it->second);
			}
		}
	}

	// Phase 2: cycle check + initial depths.
	if (!dag.recompute_depths_or_detect_cycle()) {
		// Declared cycle — fatal. Log and bail; schedule stays in last good state.
		built_ = false;
		stages_.clear();
		schedule_hash_ = 0;
		return;
	}

	// Phase 3: collect conflict pairs (alive only, registration_index < other) where
	// access overlaps with at least one Write and there's no path either direction yet.
	struct ConflictPair {
		uint32_t a;
		uint32_t b;
	};
	std::vector<ConflictPair> conflicts;
	for (uint32_t i = 0; i < N; ++i) {
		if (!registry[i].alive) {
			continue;
		}
		for (uint32_t j = i + 1; j < N; ++j) {
			if (!registry[j].alive) {
				continue;
			}
			if (!access_overlaps(
					std::span<ComponentAccess const>(registry[i].access),
					std::span<ComponentAccess const>(registry[j].access))) {
				continue;
			}
			conflicts.push_back(ConflictPair { i, j });
		}
	}

	// Sort conflicts deterministically by (min_id, max_id) where id = system_type_id_t.
	std::sort(conflicts.begin(), conflicts.end(), [&](ConflictPair const& a, ConflictPair const& b) {
		system_type_id_t const a_lo = std::min(registry[a.a].type_id, registry[a.b].type_id);
		system_type_id_t const a_hi = std::max(registry[a.a].type_id, registry[a.b].type_id);
		system_type_id_t const b_lo = std::min(registry[b.a].type_id, registry[b.b].type_id);
		system_type_id_t const b_hi = std::max(registry[b.a].type_id, registry[b.b].type_id);
		if (a_lo != b_lo) {
			return a_lo < b_lo;
		}
		return a_hi < b_hi;
	});

	// Phase 4: auto-orient each conflict edge.
	for (ConflictPair const& cp : conflicts) {
		uint32_t const u = cp.a;
		uint32_t const v = cp.b;
		// Already covered by some directed path?
		if (dag.path_exists(u, v) || dag.path_exists(v, u)) {
			continue;
		}

		// Cost = depth-increase to the target. Smaller is better.
		uint32_t const cost_u_to_v = (dag.depth[u] + 1 > dag.depth[v]) ? (dag.depth[u] + 1 - dag.depth[v]) : 0u;
		uint32_t const cost_v_to_u = (dag.depth[v] + 1 > dag.depth[u]) ? (dag.depth[v] + 1 - dag.depth[u]) : 0u;

		uint32_t first = u;
		uint32_t second = v;
		if (cost_v_to_u < cost_u_to_v) {
			first = v;
			second = u;
		} else if (cost_u_to_v == cost_v_to_u) {
			// Tiebreaker: lower system_type_id_t runs first (FNV-1a stable).
			if (registry[v].type_id < registry[u].type_id) {
				first = v;
				second = u;
			}
		}

		// Try the chosen direction. If it would cycle, try the reverse. If both cycle —
		// the conflict is genuinely unresolvable.
		dag.add_edge(first, second);
		if (!dag.recompute_depths_or_detect_cycle()) {
			// Roll back the just-added edge and try the other direction.
			auto& outs = dag.out_edges[first];
			outs.erase(std::remove(outs.begin(), outs.end(), second), outs.end());
			auto& ins = dag.in_edges[second];
			ins.erase(std::remove(ins.begin(), ins.end(), first), ins.end());

			dag.add_edge(second, first);
			if (!dag.recompute_depths_or_detect_cycle()) {
				// Both directions form cycles. Hard error.
				built_ = false;
				stages_.clear();
				schedule_hash_ = 0;
				return;
			}
		}
	}

	// Phase 5: stable topological sort with priority queue keyed by (depth, type_id ascending).
	struct PQEntry {
		uint32_t depth;
		system_type_id_t type_id;
		uint32_t reg_idx;
		bool operator>(PQEntry const& rhs) const {
			if (depth != rhs.depth) {
				return depth > rhs.depth;
			}
			return type_id > rhs.type_id;
		}
	};
	std::vector<uint32_t> indeg(N, 0);
	for (std::size_t v = 0; v < N; ++v) {
		indeg[v] = static_cast<uint32_t>(dag.in_edges[v].size());
	}
	std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;
	for (uint32_t i = 0; i < N; ++i) {
		if (!registry[i].alive) {
			continue;
		}
		if (indeg[i] == 0) {
			pq.push(PQEntry { dag.depth[i], registry[i].type_id, i });
		}
	}

	std::vector<uint32_t> order;
	while (!pq.empty()) {
		PQEntry const top = pq.top();
		pq.pop();
		order.push_back(top.reg_idx);
		for (uint32_t v : dag.out_edges[top.reg_idx]) {
			if (!registry[v].alive) {
				continue;
			}
			if (--indeg[v] == 0) {
				pq.push(PQEntry { dag.depth[v], registry[v].type_id, v });
			}
		}
	}

	// Phase 6: stage layout. A new stage starts whenever the depth changes from the
	// previous emitted system. Within each stage, all systems are conflict-free (because
	// every conflict pair has had a directed edge added, forcing them to different depths
	// or providing transitive ordering — verified at depth-recompute time).
	stages_.clear();
	for (uint32_t reg_idx : order) {
		uint32_t const d = dag.depth[reg_idx];
		if (stages_.empty() || dag.depth[stages_.back().registration_indices.front()] != d) {
			stages_.push_back(ScheduledStage {});
		}
		stages_.back().registration_indices.push_back(reg_idx);
	}

	// Phase 7: schedule_hash — FNV-1a over (stage_idx, system_type_id_t) pairs.
	uint64_t h = 0xcbf29ce484222325ULL;
	auto fold_byte = [&](uint8_t b) {
		h ^= b;
		h *= 0x100000001b3ULL;
	};
	auto fold_u64 = [&](uint64_t v) {
		for (int i = 0; i < 8; ++i) {
			fold_byte(static_cast<uint8_t>((v >> (i * 8)) & 0xffULL));
		}
	};
	for (std::size_t s = 0; s < stages_.size(); ++s) {
		fold_u64(static_cast<uint64_t>(s));
		for (uint32_t reg_idx : stages_[s].registration_indices) {
			fold_u64(registry[reg_idx].type_id);
		}
	}
	schedule_hash_ = h;

	built_ = true;
}

namespace {
	// Work item descriptor for the multi-system-stage parallel branch. The scheduler
	// builds a flat list mixing per-chunk SystemThreaded items and per-system plain
	// System<> items, then dispatches via ONE outer parallel_for. No nested parallel_for
	// (which would deadlock workers blocked in cv.wait while inner jobs sit unowned), no
	// reliance on World::current_system_registration_ (which can't be a single pointer
	// across concurrent systems).
	enum class WorkKind : uint8_t {
		ThreadedChunk, // SystemThreaded — one chunk's rows iterated with the chunk's CB.
		SerialWhole // Plain System<> — its whole tick_all body executed on one worker.
	};

	struct WorkItem {
		WorkKind kind;
		uint32_t reg_idx; // index into the registry
		uint32_t archetype_idx; // ThreadedChunk only
		uint32_t chunk_idx; // ThreadedChunk only
		uint32_t chunk_local_idx; // ThreadedChunk only — index into this system's per_chunk_cmds_
	};

	// Per-system bookkeeping built during work-item construction so the post-join merge
	// pass knows how many of each system's per_chunk_cmds_ slots actually carry work.
	struct PerSystemThreadedInfo {
		uint32_t reg_idx;
		uint32_t threaded_chunk_count;
	};
}

void SystemScheduler::run(
	World& world, Date today, std::vector<SystemRegistration>& registry,
	EcsThreadPool& pool, bool serial_mode
) {
	if (!built_) {
		return;
	}

	for (ScheduledStage const& stage : stages_) {
		if (stage.registration_indices.empty()) {
			continue;
		}

		// Execute every system in the stage. Serial mode or single-system stages run on
		// the calling thread with current_system_registration_ set, so SystemThreaded
		// systems take the existing dispatch_threaded path (one parallel_for over their
		// own chunks). Multi-system stages take the new combined-work-item path below.
		if (serial_mode || stage.registration_indices.size() == 1) {
			for (uint32_t reg_idx : stage.registration_indices) {
				SystemRegistration& reg = registry[reg_idx];
				if (!reg.alive || reg.tick_all_fn == nullptr) {
					continue;
				}
				world.set_current_registration_(&reg);
				TickContext ctx { world, today, *reg.pending_cmd };
				reg.tick_all_fn(reg.instance, world, ctx);
			}
		} else {
			// Multi-system stage parallel branch.
			//
			// Step 1: prewarm the World's query cache on the main thread for every
			// system's iteration query. Inside the parallel section, World::for_each /
			// collect_matching_chunks will read query_cache without ever mutating it
			// (matching keys are present from this prewarm pass), so plain System<>'s
			// dispatch_serial running on a worker thread is safe and SystemThreaded's
			// collect_chunks call below is safe even though we already did it on the
			// main thread.
			//
			// `resolve_query_cache_for_threaded` is the same body as the private
			// `resolve_query_cache` — non-const-thread-safe because of the `mutable`
			// hashmap. Calling it here (single-threaded) populates / refreshes the
			// cache entry. From here on the cache only needs to be READ.
			for (uint32_t reg_idx : stage.registration_indices) {
				SystemRegistration& reg = registry[reg_idx];
				if (!reg.alive) {
					continue;
				}
				if (!reg.tick_query_require_ids.empty()) {
					QueryCacheKey key { reg.tick_query_require_ids, reg.tick_query_exclude_ids };
					(void) world.resolve_query_cache_for_threaded(key);
				}
			}

			// Step 2: build the combined work-item list. Iteration order:
			// `stage.registration_indices` ascending → per system, chunks in
			// (archetype_idx, chunk_idx) ascending (the deterministic order
			// `collect_matching_chunks` returns). This guarantees the post-join
			// merge below splices per-chunk CBs into pending_cmd in
			// chunk_local_idx ascending order regardless of worker scheduling.
			std::vector<WorkItem> work_items;
			std::vector<PerSystemThreadedInfo> threaded_systems;
			for (uint32_t reg_idx : stage.registration_indices) {
				SystemRegistration& reg = registry[reg_idx];
				if (!reg.alive || reg.tick_all_fn == nullptr) {
					continue;
				}

				if (reg.is_threaded && reg.collect_chunks_fn != nullptr
					&& reg.per_chunk_cmds_accessor != nullptr
					&& reg.tick_one_chunk_fn != nullptr) {
					std::vector<ChunkLocation> chunks = reg.collect_chunks_fn(world);
					std::vector<CommandBuffer>* cbs = reg.per_chunk_cmds_accessor(reg.instance);
					if (cbs->size() < chunks.size()) {
						cbs->resize(chunks.size());
					}
					// Clear + arm parallel mode on the slots we're about to use this tick.
					// Tail slots from a previous wider tick are left untouched — they
					// hold no live state because their merge_from already cleared them.
					for (std::size_t i = 0; i < chunks.size(); ++i) {
						(*cbs)[i].clear();
						(*cbs)[i].set_parallel_mode(true);
					}
					for (std::size_t i = 0; i < chunks.size(); ++i) {
						WorkItem item;
						item.kind = WorkKind::ThreadedChunk;
						item.reg_idx = reg_idx;
						item.archetype_idx = chunks[i].archetype_idx;
						item.chunk_idx = chunks[i].chunk_idx;
						item.chunk_local_idx = static_cast<uint32_t>(i);
						work_items.push_back(item);
					}
					threaded_systems.push_back(
						PerSystemThreadedInfo { reg_idx, static_cast<uint32_t>(chunks.size()) }
					);
				} else {
					// Plain System<> (or a SystemThreaded with no entry points — shouldn't
					// happen post-Phase-0). One whole-tick work item; dispatch_serial
					// inside tick_all does not depend on current_system_registration_.
					WorkItem item;
					item.kind = WorkKind::SerialWhole;
					item.reg_idx = reg_idx;
					item.archetype_idx = 0;
					item.chunk_idx = 0;
					item.chunk_local_idx = 0;
					work_items.push_back(item);
				}
			}

			// Step 3: outer parallel_for. Each work item runs straight-line code — no
			// nested parallel_for, no run_concurrent — so the pool's per-call DoneState
			// is the only counter touched and workers never block on inner dispatches.
			if (!work_items.empty()) {
				pool.parallel_for(work_items.size(),
					[&work_items, &registry, &world, today]
					(std::size_t i, uint32_t /*worker_id*/) {
						WorkItem const& item = work_items[i];
						SystemRegistration& reg = registry[item.reg_idx];
						if (item.kind == WorkKind::ThreadedChunk) {
							std::vector<CommandBuffer>* cbs
								= reg.per_chunk_cmds_accessor(reg.instance);
							TickContext ctx { world, today, (*cbs)[item.chunk_local_idx] };
							reg.tick_one_chunk_fn(
								reg.instance, world, ctx,
								item.archetype_idx, item.chunk_idx
							);
						} else {
							TickContext ctx { world, today, *reg.pending_cmd };
							reg.tick_all_fn(reg.instance, world, ctx);
						}
					}
				);
			}

			// Step 4: per-SystemThreaded merge pass. Walk each threaded system's used
			// per_chunk_cmds_ slots in chunk_local_idx ascending order, splice into
			// pending_cmd. Plain System<> systems wrote directly to pending_cmd —
			// nothing to merge for them.
			for (PerSystemThreadedInfo const& info : threaded_systems) {
				SystemRegistration& reg = registry[info.reg_idx];
				std::vector<CommandBuffer>* cbs = reg.per_chunk_cmds_accessor(reg.instance);
				for (uint32_t i = 0; i < info.threaded_chunk_count; ++i) {
					(*cbs)[i].set_parallel_mode(false);
					reg.pending_cmd->merge_from(std::move((*cbs)[i]));
				}
			}
		}

		// Stage barrier: apply each system's pending CommandBuffer in registration_index
		// ascending order. World's in_tick_ flag is still true here (we only clear it
		// after all stages complete), but in_apply_phase_ is set around the apply loop
		// so the in-tick mutation guard yields and the queued ops actually execute.
		world.set_in_apply_phase_(true);
		for (uint32_t reg_idx : stage.registration_indices) {
			SystemRegistration& reg = registry[reg_idx];
			if (reg.pending_cmd != nullptr) {
				reg.pending_cmd->apply(world);
			}
		}
		world.set_in_apply_phase_(false);
	}
}
