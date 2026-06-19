#pragma once

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/AggregatePopIncomeSystem.hpp"
#include "openvic-simulation/ecs_rgo/ApplyEmployeeIncomeToPopsSystem.hpp"
#include "openvic-simulation/ecs_rgo/ApplyOwnerIncomeToPopsSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputeEmployeeIncomeSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputeOwnerIncomeSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputePopulationTotalsSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoHireSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoProduceAndPlaceOrderSystem.hpp"
#include "openvic-simulation/ecs_rgo/RgoResolveSellOrderAndOwnerShareSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Registers every RGO system in pipeline order. Tests call this once after setting up
	// singletons + entities. Registration order matches the stage order for documentation
	// purposes — the actual stage placement is driven by `declared_run_after` on each system.
	inline void register_all_rgo_systems(ecs::World& world) {
		world.register_system<RgoComputePopulationTotalsSystem>();
		world.register_system<RgoHireSystem>();
		world.register_system<RgoProduceAndPlaceOrderSystem>();
		world.register_system<RgoResolveSellOrderAndOwnerShareSystem>();
		world.register_system<RgoComputeOwnerIncomeSystem>();
		world.register_system<RgoComputeEmployeeIncomeSystem>();
		world.register_system<ApplyEmployeeIncomeToPopsSystem>();
		world.register_system<ApplyOwnerIncomeToPopsSystem>();
		world.register_system<AggregatePopIncomeSystem>();
	}
}
