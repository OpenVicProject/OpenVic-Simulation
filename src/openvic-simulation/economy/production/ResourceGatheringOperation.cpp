#include "ResourceGatheringOperation.hpp"
#include "ResourceGatheringOperationDeps.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

ResourceGatheringOperation::ResourceGatheringOperation(
	ResourceGatheringOperationDeps const& rgo_deps,
	ProductionType const* new_production_type_nullable,
	fixed_point_t new_size_multiplier,
	fixed_point_t new_revenue_yesterday,
	fixed_point_t new_output_quantity_yesterday,
	fixed_point_t new_unsold_quantity_yesterday,
	memory::vector<Employee>&& new_employees
)
	: market_instance { rgo_deps.market_instance }, modifier_effect_cache { rgo_deps.modifier_effect_cache },
	  production_type_nullable { new_production_type_nullable }, revenue_yesterday { new_revenue_yesterday },
	  output_quantity_yesterday { new_output_quantity_yesterday }, unsold_quantity_yesterday { new_unsold_quantity_yesterday },
	  size_multiplier { new_size_multiplier }, employees { std::move(new_employees) }, employee_count_per_type_cache { rgo_deps.pop_type_keys } {}

ResourceGatheringOperation::ResourceGatheringOperation(
	ResourceGatheringOperationDeps const& rgo_deps
) : ResourceGatheringOperation {
	rgo_deps,
	nullptr, 0,
	0, 0,
	0, {}
} {}

pop_size_t ResourceGatheringOperation::get_employee_count_per_type_cache(PopType const& key) const {
	return employee_count_per_type_cache.at(key);
}

void ResourceGatheringOperation::setup_location_ptr(ProvinceInstance& location) {
	if (location_ptr != nullptr) {
		spdlog::error_s("RGO already has a location_ptr pointing to province {}", *location_ptr);
	}

	location_ptr = &location;
}

void ResourceGatheringOperation::initialise_rgo_size_multiplier() {
	if (production_type_nullable == nullptr) {
		size_multiplier = 0;
		max_employee_count_cache = 0;
		return;
	}

	ProvinceInstance& location = *location_ptr;
	ProductionType const& production_type = *production_type_nullable;
	std::span<const Job> jobs = production_type.get_jobs();

	pop_size_t total_worker_count_in_province = 0; //not counting equivalents
	for (Job const& job : jobs) {
		total_worker_count_in_province += location.get_population_by_type(*job.get_pop_type());
	}

	const fixed_point_t size_modifier = calculate_size_modifier();
	const fixed_point_t base_workforce_size = production_type.get_base_workforce_size();
	if (size_modifier == 0) {
		size_multiplier = 0;
	} else {
		size_multiplier = ((total_worker_count_in_province / (size_modifier * base_workforce_size)).ceil() * fixed_point_t::_1_50).floor();
	}

	max_employee_count_cache = (size_modifier * size_multiplier * base_workforce_size).floor<pop_size_t>();
}

fixed_point_t ResourceGatheringOperation::calculate_size_modifier() const {
	if (production_type_nullable == nullptr) {
		return 1;
	}
	ProvinceInstance& location = *location_ptr;
	ProductionType const& production_type = *production_type_nullable;

	fixed_point_t size_modifier = 1;
	if (production_type.get_is_farm_for_tech()) {
		size_modifier += location.get_modifier_effect_value(*modifier_effect_cache.get_farm_rgo_size_global());
	}

	if (production_type.get_is_farm_for_non_tech()) {
		size_modifier += location.get_modifier_effect_value(*modifier_effect_cache.get_farm_rgo_size_local());
	}

	if (production_type.get_is_mine_for_tech()) {
		size_modifier += location.get_modifier_effect_value(*modifier_effect_cache.get_mine_rgo_size_global());
	}

	if (production_type.get_is_mine_for_non_tech()) {
		size_modifier += location.get_modifier_effect_value(*modifier_effect_cache.get_mine_rgo_size_local());
	}

	size_modifier += location.get_modifier_effect_value(
		*modifier_effect_cache.get_good_effects(production_type.get_output_good()).get_rgo_size()
	);
	return size_modifier > 0 ? size_modifier : fixed_point_t::_0;
}

void ResourceGatheringOperation::rgo_tick(memory::vector<fixed_point_t>& reusable_vector) {
	ProvinceInstance& location = *location_ptr;
	if (production_type_nullable == nullptr || location.get_owner() == nullptr) {
		output_quantity_yesterday = 0;
		revenue_yesterday = 0;
		return;
	}

	ProductionType const& production_type = *production_type_nullable;
	std::span<const Job> jobs = production_type.get_jobs();

	total_worker_count_in_province_cache = 0; //not counting equivalents
	for (Job const& job : jobs) {
		total_worker_count_in_province_cache += location.get_population_by_type(*job.get_pop_type());
	}

	hire();

	total_owner_count_in_state_cache = 0;
	owner_pops_cache_nullable = nullptr;

	if (production_type.get_owner().has_value()) {
		PopType const& owner_pop_type = *production_type.get_owner()->get_pop_type();
		total_owner_count_in_state_cache = location.get_state()->get_population_by_type(owner_pop_type);
		owner_pops_cache_nullable = &location.get_state()->get_pops_cache_by_type(owner_pop_type);
	}

	output_quantity_yesterday = produce();
	if (output_quantity_yesterday > 0) {
		CountryInstance* const country_to_report_economy_nullable = location.get_country_to_report_economy();
		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_output(production_type, output_quantity_yesterday);
		}

		market_instance.place_market_sell_order(
			{
				production_type.get_output_good(),
				country_to_report_economy_nullable,
				output_quantity_yesterday,
				this,
				after_sell,
			},
			reusable_vector
		);
	}
}

void ResourceGatheringOperation::after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) {
	ResourceGatheringOperation& rgo = *static_cast<ResourceGatheringOperation*>(actor);
	rgo.revenue_yesterday = sell_result.get_money_gained();
	rgo.pay_employees(reusable_vector);
}

void ResourceGatheringOperation::hire() {
	pop_size_t const& available_worker_count = total_worker_count_in_province_cache;
	total_employees_count_cache = 0;
	total_paid_employees_count_cache = 0;
	employees.clear(); //TODO implement Victoria 2 hiring logic
	employee_count_per_type_cache.fill(0);
	if (production_type_nullable == nullptr) {
		return;
	}
	ProvinceInstance& location = *location_ptr;

	ProductionType const& production_type = *production_type_nullable;
	if (max_employee_count_cache <= 0) { return; }
	if (available_worker_count <= 0) { return; }

	fixed_point_t proportion_to_hire;
	if (max_employee_count_cache >= available_worker_count) {
		//hire everyone
		proportion_to_hire = 1;
	} else {
		//hire all pops proportionally
		const fixed_point_t max_worker_count_real = max_employee_count_cache, available_worker_count_real = available_worker_count;
		proportion_to_hire = max_worker_count_real / available_worker_count_real;
	}

	std::span<const Job> jobs = production_type.get_jobs();
	for (Pop& pop : location.get_mutable_pops()){
		PopType const& pop_type = *pop.get_type();
		for (Job const& job : jobs) {
			PopType const* const job_pop_type = job.get_pop_type();
			if (job_pop_type && *job_pop_type == pop_type) {
				const pop_size_t pop_size_to_hire = static_cast<pop_size_t>((proportion_to_hire * pop.get_size()).floor());
				if (pop_size_to_hire <= 0) {
					continue;
				}

				employee_count_per_type_cache.at(pop_type) += pop_size_to_hire;
				employees.emplace_back(pop, pop_size_to_hire);
				pop.hire(pop_size_to_hire);
				total_employees_count_cache += pop_size_to_hire;
				if (!pop_type.get_is_slave()) {
					total_paid_employees_count_cache += pop_size_to_hire;
				}
				break;
			}
		}
	}
}

fixed_point_t ResourceGatheringOperation::produce() {
	const fixed_point_t size_modifier = calculate_size_modifier();
	if (size_modifier == 0){
		return 0;
	}

	if (production_type_nullable == nullptr || max_employee_count_cache <= 0) {
		return 0;
	}

	ProvinceInstance& location = *location_ptr;

	ProductionType const& production_type = *production_type_nullable;
	fixed_point_t throughput_multiplier = 1;
	fixed_point_t output_multiplier = 1;

	std::optional<Job> const& owner = production_type.get_owner();
	if (owner.has_value()) {
		State const* state_ptr = location.get_state();
		if (state_ptr == nullptr) {
			spdlog::error_s("Province {} has no state.", location);
			return 0;
		}

		State const& state = *state_ptr;
		const pop_size_t state_population = state.get_total_population();
		Job const& owner_job = owner.value();

		if (total_owner_count_in_state_cache > 0) {
			switch (owner_job.get_effect_type()) {
				case Job::effect_t::OUTPUT:
					output_multiplier += owner_job.get_effect_multiplier() * total_owner_count_in_state_cache / state_population;
					break;
				case Job::effect_t::THROUGHPUT:
					throughput_multiplier += owner_job.get_effect_multiplier() * total_owner_count_in_state_cache / state_population;
					break;
				default:
					spdlog::error_s("Invalid job effect in RGO {}", production_type);
					break;
			}
		}
	}

	// TODO - work out how best to avoid repeated lookups of the same effects,
	// e.g. by caching total non-local effect values at the CountryInstance level
	throughput_multiplier += location.get_modifier_effect_value(*modifier_effect_cache.get_rgo_throughput_tech())
		+ location.get_modifier_effect_value(*modifier_effect_cache.get_rgo_throughput_country())
		+ location.get_modifier_effect_value(*modifier_effect_cache.get_local_rgo_throughput());
	output_multiplier += location.get_modifier_effect_value(*modifier_effect_cache.get_rgo_output_tech())
		+ location.get_modifier_effect_value(*modifier_effect_cache.get_rgo_output_country())
		+ location.get_modifier_effect_value(*modifier_effect_cache.get_local_rgo_output());

	if (production_type.get_is_farm_for_tech()) {
		const fixed_point_t farm_rgo_throughput_and_output =
			location.get_modifier_effect_value(*modifier_effect_cache.get_farm_rgo_throughput_and_output());
		throughput_multiplier += farm_rgo_throughput_and_output;
		output_multiplier += farm_rgo_throughput_and_output;
	}

	if (production_type.get_is_farm_for_non_tech()) {
		output_multiplier += location.get_modifier_effect_value(*modifier_effect_cache.get_farm_rgo_output_global())
			+ location.get_modifier_effect_value(*modifier_effect_cache.get_farm_rgo_output_local());
	}

	if (production_type.get_is_mine_for_tech()) {
		const fixed_point_t mine_rgo_throughput_and_output =
			location.get_modifier_effect_value(*modifier_effect_cache.get_mine_rgo_throughput_and_output());
		throughput_multiplier += mine_rgo_throughput_and_output;
		output_multiplier += mine_rgo_throughput_and_output;
	}

	if (production_type.get_is_mine_for_non_tech()) {
		output_multiplier += location.get_modifier_effect_value(*modifier_effect_cache.get_mine_rgo_output_global())
			+ location.get_modifier_effect_value(*modifier_effect_cache.get_mine_rgo_output_local());
	}

	auto const& good_effects = modifier_effect_cache.get_good_effects(production_type.get_output_good());
	throughput_multiplier += location.get_modifier_effect_value(*good_effects.get_rgo_goods_throughput());
	output_multiplier += location.get_modifier_effect_value(*good_effects.get_rgo_goods_output());

	fixed_point_t throughput_from_workers = 0;
	fixed_point_t output_from_workers = 1;

	for (auto const& [pop_type, employees_of_type] : employee_count_per_type_cache) {
		for (Job const& job : production_type.get_jobs()) {
			if (job.get_pop_type() != &pop_type) {
				continue;
			}

			const fixed_point_t effect_multiplier = job.get_effect_multiplier();
			fixed_point_t relative_to_workforce =
				fixed_point_t::parse(employees_of_type) / fixed_point_t::parse(max_employee_count_cache);
			const fixed_point_t amount = job.get_amount();
			if (effect_multiplier != fixed_point_t::_1 && relative_to_workforce > amount) {
				relative_to_workforce = amount;
			}
			switch (job.get_effect_type()) {
				case Job::effect_t::OUTPUT:
					output_from_workers += effect_multiplier * relative_to_workforce;
					break;
				case Job::effect_t::THROUGHPUT:
					throughput_from_workers += effect_multiplier * relative_to_workforce;
					break;
				default:
					spdlog::error_s("Invalid job effect in RGO {}", production_type);
					break;
			}
		}
	}

	//if province is overseas multiply by (1 + overseas penalty)

	return production_type.get_base_output_quantity()
		* size_modifier * size_multiplier
		* throughput_multiplier * throughput_from_workers
		* output_multiplier * output_from_workers;
}

void ResourceGatheringOperation::pay_employees(memory::vector<fixed_point_t>& reusable_vector) {
	ProvinceInstance& location = *location_ptr;
	fixed_point_t const& revenue = revenue_yesterday;

	total_owner_income_cache = 0;
	total_employee_income_cache = 0;
	if (revenue <= 0 || total_worker_count_in_province_cache <= 0) {
		if (revenue < 0) {
			spdlog::error_s("Negative revenue for province {}", location);
		}
		if (total_worker_count_in_province_cache < 0) {
			spdlog::error_s("Negative total worker count for province {}", location);
		}
		return;
	}

	CountryInstance* const country_to_report_economy_nullable = location.get_country_to_report_economy();
	fixed_point_t total_minimum_wage = 0;
	if (country_to_report_economy_nullable != nullptr) {
		CountryInstance& country_to_report_economy = *country_to_report_economy_nullable;
		for (Employee& employee : employees) {
			total_minimum_wage += employee.update_minimum_wage(country_to_report_economy);
		}
	}

	if (revenue <= total_minimum_wage) {
		for (Employee& employee : employees) {
			const fixed_point_t income_for_this_pop = std::max(
				fixed_point_t::mul_div(
					revenue,
					employee.get_minimum_wage_cached(),
					total_minimum_wage
				),
				fixed_point_t::epsilon //revenue > 0 is already checked, so rounding up
			);
			Pop& employee_pop = employee.get_pop();
			employee_pop.add_rgo_worker_income(income_for_this_pop);
			total_employee_income_cache += income_for_this_pop;
		}
	} else {
		fixed_point_t revenue_left = revenue;
		if (total_owner_count_in_state_cache > 0) {
			const fixed_point_t upper_limit = std::min(
				fixed_point_t::_0_50,
				fixed_point_t::_1 - total_minimum_wage / revenue_left
			);
			const fixed_point_t owner_share = std::min(
				fixed_point_t::_2 * total_owner_count_in_state_cache / total_worker_count_in_province_cache,
				upper_limit
			);

			for (Pop* owner_pop_ptr : *owner_pops_cache_nullable) {
				Pop& owner_pop = *owner_pop_ptr;
				const fixed_point_t income_for_this_pop = std::max(
					revenue_left * (owner_share * owner_pop.get_size()) / total_owner_count_in_state_cache,
					fixed_point_t::epsilon //revenue > 0 is already checked, so rounding up
				);
				owner_pop.add_rgo_owner_income(income_for_this_pop);
				total_owner_income_cache += income_for_this_pop;
			}
			revenue_left -= total_owner_income_cache;
		}

		if (total_paid_employees_count_cache == 0) {
			//scenario slaves only
			//Money is removed from system in Victoria 2.
		} else {
			memory::vector<fixed_point_t>& incomes = reusable_vector;
			incomes.resize(employees.size());

			pop_size_t count_workers_to_be_paid = total_paid_employees_count_cache;
			for (size_t i = 0; i < employees.size(); i++) {
				Employee& employee = employees[i];
				Pop& employee_pop = employee.get_pop();

				PopType const* employee_pop_type = employee_pop.get_type();
				if (employee_pop_type == nullptr) {
					spdlog::error_s("employee has nullptr pop_type.");
					return;
				}

				if (employee_pop_type->get_is_slave()) {
					continue;
				}
				
				const fixed_point_t minimum_wage = employee.get_minimum_wage_cached();
				if (minimum_wage > 0 && incomes[i] == minimum_wage) {
					continue;
				}

				const pop_size_t employee_size = employee.get_size();
				const fixed_point_t income_for_this_pop = std::max(
					revenue_left * employee_size / count_workers_to_be_paid,
					fixed_point_t::epsilon //revenue > 0 is already checked, so rounding up
				);

				if (income_for_this_pop < minimum_wage) {
					incomes[i] = minimum_wage;
					revenue_left -= minimum_wage;
					count_workers_to_be_paid -= employee_size;
					i = -1; //Restart loop and skip minimum incomes. This is required to spread the remaining revenue again.
				} else {
					incomes[i] = income_for_this_pop;
				}
			}

			for (size_t i = 0; i < employees.size(); i++) {
				Employee& employee = employees[i];
				Pop& employee_pop = employee.get_pop();
				const fixed_point_t income_for_this_pop = incomes[i];
				if (income_for_this_pop > 0) {
					employee_pop.add_rgo_worker_income(income_for_this_pop);
					total_employee_income_cache += income_for_this_pop;
				}
			}

			reusable_vector.clear();
		}
	}
}