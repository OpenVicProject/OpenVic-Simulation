#pragma once

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
		// Logs a clear error if cycles are detected (declared or auto-orientation-forced)
		// or if a conflict pair has no resolvable orientation.
		void rebuild(std::vector<SystemRegistration>& registry);

		// Execute one tick. Iterates stages in order; within a stage, dispatches systems
		// concurrently via the EcsThreadPool (or serially if `serial_mode == true` or the
		// stage has only one system). After each stage joins, applies each system's
		// pending CommandBuffer in registration_index ascending order.
		void run(
			World& world, Date today, std::vector<SystemRegistration>& registry,
			EcsThreadPool& pool, bool serial_mode
		);

		// FNV-1a hash over the (stage_index, system_type_id_t) pairs of the schedule.
		uint64_t schedule_hash() const noexcept { return schedule_hash_; }

		// Drop the built state — next `run` call will rebuild.
		void invalidate() noexcept { built_ = false; }

		bool built() const noexcept { return built_; }

	private:
		bool built_ = false;
		std::vector<ScheduledStage> stages_;
		uint64_t schedule_hash_ = 0;
	};
}
