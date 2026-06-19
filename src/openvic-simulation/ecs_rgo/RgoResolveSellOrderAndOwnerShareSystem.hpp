#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoProduceAndPlaceOrderSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 4 — resolves the sell order against the market (stub) and caches owner_share +
	// total_minimum_wage on ProvinceRgoResult.
	//
	// Writes:    ProvinceRgoResult.{revenue_yesterday, owner_share, total_minimum_wage}
	//            ProvinceRgoSellOrder.has_request → false
	//            ProvinceRgoHired (each Employee's minimum_wage_cached field)
	// Reads:     ProvinceRgoConfig, ProvinceRgoCacheTotals
	// run_after: RgoProduceAndPlaceOrderSystem
	//
	// owner_share clamp from RgoMath::compute_owner_share. Set to 0 when there are no owner
	// pops or when revenue is insufficient (revenue <= total_minimum_wage) — the latter is
	// signalled to Stage 5b which then picks the proportional-distribution branch.
	struct RgoResolveSellOrderAndOwnerShareSystem :
		ecs::SystemThreaded<RgoResolveSellOrderAndOwnerShareSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceRgoCacheTotals const& totals,
			ProvinceRgoHired& hired,
			ProvinceRgoSellOrder& order,
			ProvinceRgoResult& result
		);

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoProduceAndPlaceOrderSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoResolveSellOrderAndOwnerShareSystem)
