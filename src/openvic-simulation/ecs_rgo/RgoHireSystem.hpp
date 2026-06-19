#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputePopulationTotalsSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 2 — recomputes the per-province hire decision list.
	//
	// Writes:    ProvinceRgoHired.{employees, employee_count_per_type, total_employees,
	//                              total_paid_employees}
	// Reads:     ProvinceRgoConfig, ProvinceLocation, ProvincePopList, ProvinceRgoCacheTotals
	// Extra:     PopCore (cross-archetype)
	// run_after: RgoComputePopulationTotalsSystem
	//
	// Mirrors the legacy `ResourceGatheringOperation::hire` placeholder logic exactly:
	// hire-everyone vs proportional hiring based on max_employee_count vs available workers.
	// max_employee_count itself is set once at fixture build (mirroring the legacy
	// `initialise_rgo_size_multiplier`).
	struct RgoHireSystem : ecs::SystemThreaded<RgoHireSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceLocation const& loc,
			ProvincePopList const& pop_list,
			ProvinceRgoCacheTotals const& totals,
			ProvinceRgoHired& hired
		);

		static constexpr std::array<ecs::component_type_id_t, 1> extra_reads() {
			return { ecs::component_type_id_of<PopCore>() };
		}

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoComputePopulationTotalsSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoHireSystem)
