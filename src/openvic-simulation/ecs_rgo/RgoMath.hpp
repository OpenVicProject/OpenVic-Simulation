#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

// Pure math kernels for the RGO pipeline — header-only, no World dependency, no state. Every
// function takes raw input by value or reference and returns a value: trivially unit-testable
// outside any ECS context, and freely shareable between Stage-3 / Stage-4 / Stage-5 / etc.
// systems via the OpenVic::ecs_rgo::detail namespace.

namespace OpenVic::ecs_rgo::detail {

	// Classifies a production type into the four Vic2 farm/mine buckets using the same logic
	// as ProductionType::get_is_farm_for_tech / _for_non_tech / get_is_mine_for_tech /
	// get_is_mine_for_non_tech. With `use_simple_farm_mine_logic`, the `is_farm` / `is_mine`
	// flags are taken at face value; otherwise the Vic2 quirk applies (a production type is
	// "farm for tech" only if not also mine, and "mine for non-tech" only if not also farm).
	struct FarmMineClassification {
		bool is_farm_for_tech = false;
		bool is_farm_for_non_tech = false;
		bool is_mine_for_tech = false;
		bool is_mine_for_non_tech = false;
	};

	inline FarmMineClassification resolve_farm_mine_classification(
		ProductionTypeDef const& pt,
		RgoGameRules const& rules
	) {
		FarmMineClassification c;
		if (rules.use_simple_farm_mine_logic) {
			c.is_farm_for_tech = pt.is_farm;
			c.is_farm_for_non_tech = pt.is_farm;
			c.is_mine_for_tech = pt.is_mine;
			c.is_mine_for_non_tech = pt.is_mine;
		} else {
			c.is_farm_for_tech = pt.is_farm && !pt.is_mine;
			c.is_farm_for_non_tech = pt.is_farm;
			c.is_mine_for_tech = pt.is_mine;
			c.is_mine_for_non_tech = pt.is_mine && !pt.is_farm;
		}
		return c;
	}

	// Mirrors ResourceGatheringOperation::calculate_size_modifier. Starts at 1, accumulates
	// farm/mine size modifiers based on the resolved classification, then the per-good
	// rgo_size. Clamped at 0 — never negative.
	inline fixed_point_t calculate_size_modifier(
		ProductionTypeDef const& pt,
		ProvinceRgoFarmMineModifiers const& fm,
		ProvinceRgoGoodModifiers const& gm,
		RgoGameRules const& rules
	) {
		FarmMineClassification const cls = resolve_farm_mine_classification(pt, rules);
		fixed_point_t result = fixed_point_t::_1;
		if (cls.is_farm_for_tech) {
			result += fm.farm_size_global;
		}
		if (cls.is_farm_for_non_tech) {
			result += fm.farm_size_local;
		}
		if (cls.is_mine_for_tech) {
			result += fm.mine_size_global;
		}
		if (cls.is_mine_for_non_tech) {
			result += fm.mine_size_local;
		}
		result += gm.rgo_size;
		return result > fixed_point_t::_0 ? result : fixed_point_t::_0;
	}

	// Mirrors the legacy size-multiplier formula (the "tiered by 1.5" rounding):
	//   floor( ceil(workers / base_workforce_size / size_modifier) * 1.5 )
	// Returns 0 if size_modifier == 0.
	inline fixed_point_t calculate_size_multiplier_from_workforce(
		pop_sum_t total_workers,
		pop_size_t base_workforce_size,
		fixed_point_t size_modifier
	) {
		if (size_modifier == fixed_point_t::_0) {
			return fixed_point_t::_0;
		}
		// fp::from_fraction handles the integer→fixed_point widening exactly as the legacy. The
		// int64_t overload is selected by argument types — pop_sum_t is int64_t which exceeds
		// the integral_max_size_4 constraint on the templated overload.
		fixed_point_t const worker_fraction =
			fp::from_fraction(
				static_cast<int64_t>(total_workers), static_cast<int64_t>(base_workforce_size)
			) / size_modifier;
		return (worker_fraction.ceil() * fixed_point_t::_1_50).floor();
	}

	// max_employee_count = floor(size_modifier * size_multiplier * base_workforce_size).
	// Matches initialise_rgo_size_multiplier's final line (note the .floor<pop_size_t>()).
	inline pop_size_t calculate_max_employee_count(
		fixed_point_t size_modifier,
		fixed_point_t size_multiplier,
		pop_size_t base_workforce_size
	) {
		fixed_point_t const product =
			size_modifier * size_multiplier * fixed_point_t { base_workforce_size };
		return product.floor<pop_size_t>();
	}

	// Composes the throughput multiplier exactly as the legacy produce() does: starts at 1,
	// adds the owner-job throughput contribution (if any), the rgo_throughput_tech /
	// _country / local_rgo_throughput, the farm/mine throughput-and-output multipliers, and
	// the per-good rgo_goods_throughput. Caller supplies the owner-job piece already-scaled.
	inline fixed_point_t compose_throughput_multiplier(
		FarmMineClassification const& cls,
		ProvinceRgoModifiers const& mods,
		ProvinceRgoFarmMineModifiers const& fm,
		ProvinceRgoGoodModifiers const& gm,
		fixed_point_t owner_job_throughput_contribution
	) {
		fixed_point_t result = fixed_point_t::_1;
		result += owner_job_throughput_contribution;
		result += mods.rgo_throughput_tech;
		result += mods.rgo_throughput_country;
		result += mods.local_rgo_throughput;
		if (cls.is_farm_for_tech) {
			result += fm.farm_throughput_and_output;
		}
		if (cls.is_mine_for_tech) {
			result += fm.mine_throughput_and_output;
		}
		result += gm.rgo_goods_throughput;
		return result;
	}

	// Composes the output multiplier exactly as the legacy produce() does. Note that the
	// farm/mine non-tech contributions appear ONLY in the output multiplier (not throughput) —
	// that asymmetry is preserved here.
	inline fixed_point_t compose_output_multiplier(
		FarmMineClassification const& cls,
		ProvinceRgoModifiers const& mods,
		ProvinceRgoFarmMineModifiers const& fm,
		ProvinceRgoGoodModifiers const& gm,
		fixed_point_t owner_job_output_contribution
	) {
		fixed_point_t result = fixed_point_t::_1;
		result += owner_job_output_contribution;
		result += mods.rgo_output_tech;
		result += mods.rgo_output_country;
		result += mods.local_rgo_output;
		if (cls.is_farm_for_tech) {
			result += fm.farm_throughput_and_output;
		}
		if (cls.is_farm_for_non_tech) {
			result += fm.farm_output_global;
			result += fm.farm_output_local;
		}
		if (cls.is_mine_for_tech) {
			result += fm.mine_throughput_and_output;
		}
		if (cls.is_mine_for_non_tech) {
			result += fm.mine_output_global;
			result += fm.mine_output_local;
		}
		result += gm.rgo_goods_output;
		return result;
	}

	// One step of the legacy per-employee-type fold (per-Job loop body):
	//   fraction = employees_of_type / max_employee_count
	//   effect   = (effect_multiplier != 1 && fraction > amount)
	//                ? effect_multiplier * amount     // Vic2 capped-share special case
	//                : effect_multiplier * fraction
	// Sign-preserved.
	inline fixed_point_t compute_job_effect(
		fixed_point_t effect_multiplier,
		fixed_point_t amount,
		pop_size_t employees_of_type,
		pop_size_t max_employee_count
	) {
		if (max_employee_count <= 0) {
			return fixed_point_t::_0;
		}
		fixed_point_t const fraction =
			fp::from_fraction<pop_size_t>(employees_of_type, max_employee_count);
		if (effect_multiplier != fixed_point_t::_1 && fraction > amount) {
			return effect_multiplier * amount;
		}
		return fp::mul_div(effect_multiplier, employees_of_type, max_employee_count);
	}

	// Walks every Job in `pt` and folds the employees-of-type contribution into the
	// per-effect-type accumulators. throughput_from_workers starts at 0, output_from_workers
	// at 1 — same initial values as the legacy code.
	struct WorkersContribution {
		fixed_point_t throughput_from_workers = fixed_point_t::_0;
		fixed_point_t output_from_workers = fixed_point_t::_1;
	};

	inline WorkersContribution compute_throughput_and_output_from_workers(
		ProductionTypeDef const& pt,
		std::span<pop_size_t const> employee_count_per_type,
		pop_size_t max_employee_count
	) {
		WorkersContribution w;
		for (Job const& job : pt.jobs) {
			if (job.pop_type_idx == INVALID_IDX
				|| job.pop_type_idx >= employee_count_per_type.size()) {
				continue;
			}
			pop_size_t const employees_of_type = employee_count_per_type[job.pop_type_idx];
			if (employees_of_type <= 0) {
				continue;
			}
			fixed_point_t const effect = compute_job_effect(
				job.effect_multiplier, job.amount, employees_of_type, max_employee_count
			);
			switch (job.effect_type) {
				case JobEffectType::Output:
					w.output_from_workers += effect;
					break;
				case JobEffectType::Throughput:
					w.throughput_from_workers += effect;
					break;
				case JobEffectType::Input:
					// Legacy quirk: INPUT effect type logs an error and is ignored in produce.
					break;
			}
		}
		return w;
	}

	// Vic2 owner-share clamp: min(2 * owner_count / worker_count, min(0.5, 1 - min_wage / revenue)).
	// Caller checks total_owner_count > 0 before calling. revenue_left must be > 0.
	inline fixed_point_t compute_owner_share(
		pop_sum_t total_owner_count,
		pop_sum_t total_worker_count,
		fixed_point_t revenue_left,
		fixed_point_t total_minimum_wage
	) {
		fixed_point_t const upper_limit = std::min(
			fixed_point_t::_0_50,
			fixed_point_t::_1 - total_minimum_wage / revenue_left
		);
		fixed_point_t const desired = fp::from_fraction(
			static_cast<int64_t>(static_cast<pop_sum_t>(2) * total_owner_count),
			static_cast<int64_t>(total_worker_count)
		);
		return std::min(desired, upper_limit);
	}

	// Insufficient-revenue branch: distribute the available revenue proportionally to each
	// employee's cached minimum wage. Output is stored into out_incomes (already sized to
	// employees.size() by the caller).
	inline void distribute_insufficient_revenue_proportional(
		std::span<Employee const> employees,
		fixed_point_t revenue,
		fixed_point_t total_minimum_wage,
		std::span<fixed_point_t> out_incomes
	) {
		for (std::size_t i = 0; i < employees.size(); ++i) {
			Employee const& e = employees[i];
			fixed_point_t const proportional = fp::mul_div(
				revenue, e.minimum_wage_cached, total_minimum_wage
			);
			out_incomes[i] = std::max(proportional, fixed_point_t::epsilon);
		}
	}

	// Sufficient-revenue worker branch: the legacy min-wage-pinning algorithm rewritten as a
	// clean while(changed) loop instead of the `i = -1; continue` trick.
	//
	// Per iteration: compute every unpinned non-slave employee's proposed income; if any falls
	// below their minimum wage, pin all such employees to their minimum (deducting both their
	// minimum-wage sum from revenue_left and their headcount from paid_employee_count), then
	// restart. Terminates because each pin strictly reduces paid_employee_count.
	inline void distribute_employee_incomes_min_wage_pinning(
		std::span<Employee const> employees,
		fixed_point_t revenue_left,
		fixed_point_t total_minimum_wage,
		pop_size_t paid_employee_count,
		std::span<fixed_point_t> out_incomes
	) {
		(void) total_minimum_wage;

		std::vector<bool> pinned(employees.size(), false);
		for (std::size_t i = 0; i < employees.size(); ++i) {
			out_incomes[i] = fixed_point_t::_0;
		}

		bool changed = true;
		while (changed) {
			changed = false;
			for (std::size_t i = 0; i < employees.size(); ++i) {
				Employee const& e = employees[i];
				if (e.is_slave) {
					continue;
				}
				if (pinned[i]) {
					continue;
				}
				if (paid_employee_count <= 0) {
					break;
				}
				fixed_point_t const proposed = std::max(
					fp::mul_div(revenue_left, e.hired_size, paid_employee_count),
					fixed_point_t::epsilon
				);
				if (proposed < e.minimum_wage_cached) {
					out_incomes[i] = e.minimum_wage_cached;
					pinned[i] = true;
					revenue_left -= e.minimum_wage_cached;
					paid_employee_count -= e.hired_size;
					changed = true;
				} else {
					out_incomes[i] = proposed;
				}
			}
		}
	}
}
