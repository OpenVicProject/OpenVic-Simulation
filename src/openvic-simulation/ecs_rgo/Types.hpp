#pragma once

#include <cstdint>
#include <limits>

#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

// Local typedefs for the ecs_rgo reference implementation. Kept as plain integer aliases
// (rather than reusing the strong-typedef'd pop_size_t / pop_sum_t from the population
// subsystem) so this implementation can compile without dragging in the legacy Pop / PopType
// definitions — the whole point of the parallel reference is to demonstrate the ECS shape on
// flat POD components.
namespace OpenVic::ecs_rgo {
	using pop_size_t = int32_t;
	using pop_sum_t = int64_t;
	using pop_type_idx_t = uint32_t;
	using good_idx_t = uint32_t;
	using production_type_idx_t = uint32_t;

	inline constexpr uint32_t INVALID_IDX = std::numeric_limits<uint32_t>::max();

	using ecs::EntityID;
	using OpenVic::fixed_point_t;

	// Effect category for a Job — matches the legacy Job::effect_t. INPUT is included so the
	// produce() path can match the legacy "log error and ignore" behaviour quirk.
	enum class JobEffectType : uint8_t {
		Throughput = 0,
		Output = 1,
		Input = 2
	};

	// Job classification for a production type — only RGO is exercised by this implementation,
	// but FACTORY / ARTISAN are present so set_production_type_nullable's template-type check
	// can log-and-proceed exactly like the legacy version.
	enum class TemplateType : uint8_t {
		Factory = 0,
		Rgo = 1,
		Artisan = 2
	};

	struct Job {
		pop_type_idx_t pop_type_idx = INVALID_IDX;
		JobEffectType effect_type = JobEffectType::Throughput;
		fixed_point_t effect_multiplier {};
		fixed_point_t amount {};
		bool is_slave = false;
	};

	// One hired chunk of a pop, written into ProvinceRgoHired::employees each tick.
	// minimum_wage_cached is filled in pay_employees (now: RgoComputeEmployeeIncomeSystem).
	struct Employee {
		EntityID pop_id {};
		pop_type_idx_t pop_type_idx = INVALID_IDX;
		pop_size_t hired_size {};
		bool is_slave = false;
		fixed_point_t minimum_wage_cached {};
	};
}
