#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic::ecs {
	struct World;
	class EcsThreadPool;

	struct ScheduledStage {
		// Indices into the World's `system_registry_` of every system in this stage. All
		// systems in a stage are guaranteed conflict-free; they may execute concurrently.
		std::vector<uint32_t> registration_indices;
	};

	// Owns the DAG, stage layout, and schedule-hash for a World's set of registered
	// systems. Built lazily on the first `tick_systems` after registration changes.
	class SystemScheduler {
	public:
		// Build (or rebuild) the schedule from the current set of registered systems.
		// A detected cycle (declared or auto-orientation-forced) or a conflict pair with
		// no resolvable orientation is fatal: the schedule is cleared (no stages,
		// schedule_hash() == 0) and tick_systems runs nothing. No error is logged; the
		// zeroed hash / stage count are the observable signals.
		void rebuild(std::vector<SystemRegistration>& registry);

		// Execute one tick. Iterates stages in order; within a stage, dispatches systems
		// concurrently via the EcsThreadPool (or serially if `serial_mode == true` or the
		// stage has only one system). After each stage joins, applies each system's
		// pending CommandBuffer in the stage's deterministic emit order — ascending
		// system_type_id_t within the stage, independent of registration order.
		void run(
			World& world, Date today, std::vector<SystemRegistration>& registry,
			EcsThreadPool& pool, bool serial_mode
		);

		// FNV-1a hash over the (stage_index, system_type_id_t) pairs of the schedule.
		uint64_t schedule_hash() const noexcept { return schedule_hash_; }

		// Drop the built state — next `run` call will rebuild.
		void invalidate() noexcept { built_ = false; }

		bool built() const noexcept { return built_; }

		// Test/introspection only. Number of stages in the built schedule (0 if not built).
		std::size_t stage_count() const noexcept { return stages_.size(); }

		// Test/introspection only. Stage index of the system registered with `type_id`, or
		// SIZE_MAX if it isn't scheduled. Walks `registry` to map a stage's registration
		// indices back to type ids — lets tests assert two systems do (or do not) share a
		// stage, which is exactly what the disjoint-iteration conflict override changes.
		std::size_t stage_index_of(
			system_type_id_t type_id, std::vector<SystemRegistration> const& registry
		) const noexcept;

	private:
		bool built_ = false;
		std::vector<ScheduledStage> stages_;
		uint64_t schedule_hash_ = 0;
	};
}
