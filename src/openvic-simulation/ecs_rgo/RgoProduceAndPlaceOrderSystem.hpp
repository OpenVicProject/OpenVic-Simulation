#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoHireSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 3 — runs produce() then places a sell order.
	//
	// Writes:    ProvinceRgoResult.output_quantity_yesterday, ProvinceRgoSellOrder
	// Reads:     ProvinceRgoConfig, ProvinceRgoHired, ProvinceRgoModifiers,
	//            ProvinceRgoFarmMineModifiers, ProvinceRgoGoodModifiers, ProvinceLocation,
	//            ProvinceRgoCacheTotals
	// Extra:     StateOwnerPopList (cross-archetype — for state_population)
	// run_after: RgoHireSystem
	//
	// Recomputes size_modifier (the legacy `calculate_size_modifier`) each tick — same as the
	// legacy produce() does — then composes throughput / output multipliers and folds in the
	// per-pop-type employee contribution.
	struct RgoProduceAndPlaceOrderSystem : ecs::SystemThreaded<RgoProduceAndPlaceOrderSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceRgoHired const& hired,
			ProvinceRgoModifiers const& mods,
			ProvinceRgoFarmMineModifiers const& fm,
			ProvinceRgoGoodModifiers const& gm,
			ProvinceLocation const& loc,
			ProvinceRgoCacheTotals const& totals,
			ProvinceRgoResult& result,
			ProvinceRgoSellOrder& order
		);

		static constexpr std::array<ecs::component_type_id_t, 1> extra_reads() {
			return { ecs::component_type_id_of<StateOwnerPopList>() };
		}

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoHireSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoProduceAndPlaceOrderSystem)
