#include "CountryEconomy.hpp"

#include <mutex>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/country/CountryInstanceDeps.hpp"
#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/defines/EconomyDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSize.hpp"

using namespace OpenVic;

CountryEconomy::good_data_t::good_data_t()
	: mutex { memory::make_unique<std::mutex>() }
	{ }

void CountryEconomy::good_data_t::clear_daily_recorded_data() {
	const std::lock_guard<std::mutex> lock_guard { *mutex };
	stockpile_change_yesterday
		= quantity_traded_yesterday
		= money_traded_yesterday
		= exported_amount
		= government_needs
		= army_needs
		= navy_needs
		= overseas_maintenance
		= factory_demand
		= pop_demand
		= available_amount
		= 0;
	need_consumption_per_pop_type.clear();
	input_consumption_per_production_type.clear();
	production_per_production_type.clear();
}

static constexpr size_t DAYS_OF_BALANCE_HISTORY = 30;

CountryEconomy::CountryEconomy(
	SharedCountryValues& new_shared_country_values,
	CountryInstanceDeps const& country_instance_deps
) : country_defines { country_instance_deps.country_defines },
	game_rules_manager { country_instance_deps.game_rules_manager },
	market_instance { country_instance_deps.market_instance },
	modifier_effect_cache { country_instance_deps.modifier_effect_cache },
	shared_country_values { new_shared_country_values },
	balance_history{DAYS_OF_BALANCE_HISTORY},
	taxable_income_by_pop_type { country_instance_deps.pop_types },
	effective_tax_rate_by_strata {
		country_instance_deps.stratas,
		[this](Strata const& strata)->auto {
			return [this,&strata](DependencyTracker& tracker)->fixed_point_t {
				return tax_efficiency.get(tracker) * tax_rate_slider_value_by_strata.at(strata).get_value(tracker);
			};
		}
	},
	administration_salary_base_by_pop_type{
		country_instance_deps.pop_types,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_administration_salary_base(tracker);
			};
		}
	},
	education_salary_base_by_pop_type{
		country_instance_deps.pop_types,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_education_salary_base(tracker);
			};
		}
	},
	military_salary_base_by_pop_type{
		country_instance_deps.pop_types,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_military_salary_base(tracker);
			};
		}
	},
	social_income_variant_base_by_pop_type{
		country_instance_deps.pop_types,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_social_income_variant_base(tracker);
			};
		}
	},
	tax_rate_slider_value_by_strata { country_instance_deps.stratas },
	goods_data { country_instance_deps.good_instances },
	corruption_cost_multiplier{[this](DependencyTracker& tracker)->fixed_point_t {
		return 2 - administrative_efficiency_from_administrators.get(tracker);
	}},
	tariff_efficiency{[this](DependencyTracker& tracker)->fixed_point_t {
		return std::min(
			fixed_point_t::_1,
			administrative_efficiency_from_administrators.get(tracker) + country_defines.get_base_country_tax_efficiency()
		);
	}},
	effective_tariff_rate{[this](DependencyTracker& tracker)->fixed_point_t {
		return administrative_efficiency_from_administrators.get(tracker) * tariff_rate_slider_value.get_value(tracker);
	}},
	projected_administration_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return administration_spending_slider_value.get_value(tracker) * projected_administration_spending_unscaled_by_slider.get(tracker);
	}},
	projected_education_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return education_spending_slider_value.get_value(tracker) * projected_education_spending_unscaled_by_slider.get(tracker);
	}},
	projected_military_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return military_spending_slider_value.get_value(tracker) * projected_military_spending_unscaled_by_slider.get(tracker);
	}},
	projected_social_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return social_spending_slider_value.get_value(tracker) * projected_social_spending_unscaled_by_slider.get(tracker);
	}},
	projected_social_spending_unscaled_by_slider{[this](DependencyTracker& tracker)->fixed_point_t {
		return projected_pensions_spending_unscaled_by_slider.get(tracker)
			+ projected_unemployment_subsidies_spending_unscaled_by_slider.get(tracker);
	}},
	projected_import_subsidies{[this](DependencyTracker& tracker)->fixed_point_t {
		return has_import_subsidies.get(tracker)
			? -effective_tariff_rate.get(tracker) * get_yesterdays_import_value(tracker)
			: fixed_point_t::_0;
	}},
	projected_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return projected_administration_spending.get(tracker)
			+ projected_education_spending.get(tracker)
			+ projected_military_spending.get(tracker)
			+ projected_social_spending.get(tracker)
			+ projected_import_subsidies.get(tracker);
	}},
	has_import_subsidies{[this](DependencyTracker& tracker)->bool {
		return effective_tariff_rate.get(tracker) < 0;
	}} {
	// Some sliders need to have their max range limits temporarily set to 1 so they can start with a value of 0.5 or 1.0.
	// The range limits will be corrected on the first gamestate update, and the values will go to the closest valid point.
	for (ClampedValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
		tax_rate_slider_value.set_bounds(0, 1);
		tax_rate_slider_value.set_value(fixed_point_t::_0_50);
	}

	// army, navy and construction spending have minimums defined in EconomyDefines and always have an unmodified max (1.0).
	EconomyDefines const& economy_defines = country_instance_deps.economy_defines;
	army_spending_slider_value.set_bounds(economy_defines.get_minimum_army_spending_slider_value(), 1);
	army_spending_slider_value.set_value(1);

	navy_spending_slider_value.set_bounds(economy_defines.get_minimum_navy_spending_slider_value(), 1);
	navy_spending_slider_value.set_value(1);

	construction_spending_slider_value.set_bounds(
		economy_defines.get_minimum_construction_spending_slider_value(), 1
	);
	construction_spending_slider_value.set_value(1);

	education_spending_slider_value.set_bounds(0, 1);
	education_spending_slider_value.set_value(fixed_point_t::_0_50);

	administration_spending_slider_value.set_bounds(0, 1);
	administration_spending_slider_value.set_value(fixed_point_t::_0_50);

	social_spending_slider_value.set_bounds(0, 1);
	social_spending_slider_value.set_value(1);

	military_spending_slider_value.set_bounds(0, 1);
	military_spending_slider_value.set_value(fixed_point_t::_0_50);

	tariff_rate_slider_value.set_bounds(0, 0);
	tariff_rate_slider_value.set_value(0);	
}

CountryEconomy::good_data_t& CountryEconomy::get_good_data(GoodInstance const& good_instance) {
	return goods_data.at(good_instance);
}
CountryEconomy::good_data_t const& CountryEconomy::get_good_data(GoodInstance const& good_instance) const {
	return goods_data.at(good_instance);
}
CountryEconomy::good_data_t& CountryEconomy::get_good_data(GoodDefinition const& good_definition) {
	return goods_data.at_index(good_definition.get_index());
}
CountryEconomy::good_data_t const& CountryEconomy::get_good_data(GoodDefinition const& good_definition) const {
	return goods_data.at_index(good_definition.get_index());
}

DerivedState<fixed_point_t>& CountryEconomy::get_effective_tax_rate_by_strata(Strata const& strata) {
	return effective_tax_rate_by_strata.at(strata);
}

ReadOnlyClampedValue& CountryEconomy::get_tax_rate_slider_value_by_strata(Strata const& strata) {
	return tax_rate_slider_value_by_strata.at(strata);
}
ReadOnlyClampedValue const& CountryEconomy::get_tax_rate_slider_value_by_strata(Strata const& strata) const {
	return tax_rate_slider_value_by_strata.at(strata);
}

fixed_point_t CountryEconomy::get_taxable_income_by_strata(Strata const& strata) const {
	fixed_point_t running_total = 0;
	for (auto const& [pop_type, taxable_income] : taxable_income_by_pop_type) {
		if (pop_type.get_strata() == strata) {
			running_total += taxable_income;
		}
	}
	
	return running_total;
}

void CountryEconomy::set_strata_tax_rate_slider_value(Strata const& strata, const fixed_point_t new_value) {
	tax_rate_slider_value_by_strata.at(strata).set_value(new_value);
}
void CountryEconomy::set_army_spending_slider_value(const fixed_point_t new_value) {
	army_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_navy_spending_slider_value(const fixed_point_t new_value) {
	navy_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_construction_spending_slider_value(const fixed_point_t new_value) {
	construction_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_education_spending_slider_value(const fixed_point_t new_value) {
	education_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_administration_spending_slider_value(const fixed_point_t new_value) {
	administration_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_social_spending_slider_value(const fixed_point_t new_value) {
	social_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_military_spending_slider_value(const fixed_point_t new_value) {
	military_spending_slider_value.set_value(new_value);
}
void CountryEconomy::set_tariff_rate_slider_value(const fixed_point_t new_value) {
	tariff_rate_slider_value.set_value(new_value);
}

static inline constexpr fixed_point_t nonzero_or_one(fixed_point_t const& value) {
	return value == 0 ? fixed_point_t::_1 : value;
}
void CountryEconomy::update_country_budget(
	const fixed_point_t desired_administrator_percentage,
	IndexedFlatMap<PopType, pop_size_t> const& population_by_type,
	IndexedFlatMap<PopType, pop_size_t> const& unemployed_pops_by_type,
	ordered_set<State*> const& states
) {
	const fixed_point_t min_tax = get_modifier_effect_value(*modifier_effect_cache.get_min_tax());
	const fixed_point_t max_tax = nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_tax()));

	for (ClampedValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
		tax_rate_slider_value.set_bounds(min_tax, max_tax);
	}

	// Education and administration spending sliders have no min/max modifiers

	social_spending_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_social_spending()),
		nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_social_spending()))
	);
	military_spending_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_military_spending()),
		nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_military_spending()))
	);
	tariff_rate_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_tariff()),
		get_modifier_effect_value(*modifier_effect_cache.get_max_tariff())
	);

	// TODO - make sure we properly update everything dependent on these sliders' values,
	// as they might change if their sliders' bounds shrink past their previous values.

	tax_efficiency.set(
		country_defines.get_base_country_tax_efficiency()
		+ get_modifier_effect_value(*modifier_effect_cache.get_tax_efficiency())
		+ get_modifier_effect_value(*modifier_effect_cache.get_tax_eff()) / 100
	);

	/*
	In Victoria 2, administration efficiency is updated in the UI immediately.
	However the corruption_cost_multiplier is only updated after 2 ticks.

	OpenVic immediately updates both.
	*/

	pop_size_t total_non_colonial_population = 0;
	pop_size_t administrators = 0;
	for (State const* const state_ptr : states) {
		if (state_ptr == nullptr) {
			continue;
		}

		State const& state = *state_ptr;
		if (state.is_colonial_state()) {
			continue;
		}

		IndexedFlatMap<PopType, pop_size_t> const& state_population_by_type = state.get_population_by_type();

		for (auto const& [pop_type, size] : state_population_by_type) {
			if (pop_type.get_is_administrator()) {
				administrators += size;
			}
		}
		total_non_colonial_population += state.get_total_population();
	}

	if (total_non_colonial_population == 0) {
		administrative_efficiency_from_administrators.set(fixed_point_t::_1);
		administrator_percentage.set(fixed_point_t::_0);
	} else {		
		administrator_percentage.set(fixed_point_t::parse(administrators) / total_non_colonial_population);

		const fixed_point_t desired_administrators = desired_administrator_percentage * total_non_colonial_population;
		const fixed_point_t administrative_efficiency_from_administrators_unclamped = std::min(
			fixed_point_t::mul_div(
				administrators,
				fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_administrative_efficiency()),
				desired_administrators
			)
			* (fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_administrative_efficiency_modifier())),
			fixed_point_t::_1
		);

		administrative_efficiency_from_administrators.set(
			game_rules_manager.get_prevent_negative_administration_efficiency()
			? std::max(fixed_point_t::_0, administrative_efficiency_from_administrators_unclamped)
			: administrative_efficiency_from_administrators_unclamped
		);
	}

	fixed_point_t projected_administration_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_education_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_military_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_pensions_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_unemployment_subsidies_spending_unscaled_by_slider_running_total = 0;

	for (auto const& [pop_type, size] : population_by_type) {
		projected_administration_spending_unscaled_by_slider_running_total += size * administration_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_education_spending_unscaled_by_slider_running_total += size * education_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_military_spending_unscaled_by_slider_running_total += size * military_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_pensions_spending_unscaled_by_slider_running_total += size * calculate_pensions_base(pop_type);
		projected_unemployment_subsidies_spending_unscaled_by_slider_running_total += unemployed_pops_by_type.at(pop_type)
			* calculate_unemployment_subsidies_base(pop_type);
	}

	projected_administration_spending_unscaled_by_slider.set(
		projected_administration_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_education_spending_unscaled_by_slider.set(
		projected_education_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_military_spending_unscaled_by_slider.set(
		projected_military_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_pensions_spending_unscaled_by_slider.set(
		projected_pensions_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_unemployment_subsidies_spending_unscaled_by_slider.set(
		projected_unemployment_subsidies_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
}
void CountryEconomy::country_budget_tick_before_map(
	CountryInstance const& country_instance,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	utility::forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_COUNTRY_BUDGET_TICK
	> reusable_vectors,
	memory::vector<size_t>& reusable_index_vector
) {
	//TODO AI sliders
	// + reparations + war subsidies
	// + industrial subsidies
	// + loan interest

	fixed_point_t available_funds = cash_stockpile_start_of_tick = cash_stockpile;
	const fixed_point_t projected_administration_spending_copy = projected_administration_spending.get_untracked();
	const fixed_point_t projected_education_spending_copy = projected_education_spending.get_untracked();
	const fixed_point_t projected_military_spending_copy = projected_military_spending.get_untracked();
	const fixed_point_t projected_social_spending_copy = projected_social_spending.get_untracked();
	const fixed_point_t projected_import_subsidies_copy = projected_import_subsidies.get_untracked();
	if (projected_spending.get_untracked() <= available_funds) {
		actual_administration_budget = projected_administration_spending_copy;
		actual_education_budget = projected_education_spending_copy;
		actual_military_budget = projected_military_spending_copy;
		actual_social_budget = projected_social_spending_copy;
		actual_import_subsidies_budget = projected_import_subsidies_copy;
	} else {
		//TODO try take loan (callback?)
		//update available_funds with loan

		if (available_funds < projected_education_spending_copy) {
			actual_education_budget = available_funds;
			actual_administration_budget = 0;
			actual_military_budget = 0;
			actual_social_budget = 0;
			actual_import_subsidies_budget = 0;
		} else {
			available_funds -= projected_education_spending_copy;
			actual_education_budget = projected_education_spending_copy;

			if (available_funds < projected_administration_spending_copy) {
				actual_administration_budget = available_funds;
				available_funds = 0;
				actual_military_budget = 0;
				actual_social_budget = 0;
				actual_import_subsidies_budget = 0;
			} else {
				available_funds -= projected_administration_spending_copy;
				actual_administration_budget = projected_administration_spending_copy;

				if (available_funds < projected_social_spending_copy) {
					actual_social_budget = available_funds;
					available_funds = 0;
					actual_military_budget = 0;
					actual_import_subsidies_budget = 0;
				} else {
					available_funds -= projected_social_spending_copy;
					actual_social_budget = projected_social_spending_copy;

					if (available_funds < projected_military_spending_copy) {
						actual_military_budget = available_funds;
						available_funds = 0;
						actual_import_subsidies_budget = 0;
					} else {
						available_funds -= projected_military_spending_copy;
						actual_military_budget = projected_military_spending_copy;

						actual_import_subsidies_budget = std::min(available_funds, projected_import_subsidies_copy);
						available_funds -= actual_import_subsidies_budget;
					}
				}
			}
		}
	}

	was_administration_budget_cut_yesterday = actual_administration_budget < projected_administration_spending_copy;
	was_education_budget_cut_yesterday = actual_education_budget < projected_education_spending_copy;
	was_military_budget_cut_yesterday = actual_military_budget < projected_military_spending_copy;
	was_social_budget_cut_yesterday = actual_social_budget < projected_social_spending_copy;
	was_import_subsidies_budget_cut_yesterday = actual_import_subsidies_budget < projected_import_subsidies_copy;

	manage_national_stockpile(
		country_instance,
		reusable_goods_mask,
		reusable_vectors,
		reusable_index_vector,
		available_funds
	);

	taxable_income_by_pop_type.fill(0);
	actual_administration_spending
		= actual_education_spending
		= actual_military_spending
		= actual_pensions_spending
		= actual_unemployment_subsidies_spending
		= actual_import_subsidies_spending
		= actual_tariff_income
		= actual_national_stockpile_spending
		= actual_national_stockpile_income
	 	= 0;
}

void CountryEconomy::country_budget_tick_after_map(
	CountryInstance const& country_instance,
	const Date today
) {
	fixed_point_t total_gold_production = 0;
	for (auto const& [good_instance, data] : goods_data) {
		GoodDefinition const& good_definition = good_instance.get_good_definition();
		if (!good_definition.get_is_money()) {
			continue;
		}

		for (auto const& [production_type, produced_quantity] : data.production_per_production_type) {
			if (production_type->get_template_type() != ProductionType::template_type_t::RGO) {
				continue;
			}
			total_gold_production += produced_quantity;
		}
	}

	const fixed_point_t actual_administration_spending_copy = actual_administration_spending.load();
	if (OV_unlikely(actual_administration_spending_copy > actual_administration_budget)) {
		spdlog::error_s(
			"Country {} has overspend on administration. Spending {} instead of the allocated {}. This indicates a severe bug in the economy code.",
			country_instance, actual_administration_spending_copy, actual_administration_budget
		);
	}
	cash_stockpile -= actual_administration_spending;

	const fixed_point_t actual_education_spending_copy = actual_education_spending.load();
	if (OV_unlikely(actual_education_spending_copy > actual_education_budget)) {
		spdlog::error_s(
			"Country {} has overspend on education. Spending {} instead of the allocated {}. This indicates a severe bug in the economy code.",
			country_instance, actual_education_spending_copy, actual_education_budget
		);
	}
	cash_stockpile -= actual_education_spending;

	const fixed_point_t actual_military_spending_copy = actual_military_spending.load();
	if (OV_unlikely(actual_military_spending_copy > actual_military_budget)) {
		spdlog::error_s(
			"Country {} has overspend on military. Spending {} instead of the allocated {}. This indicates a severe bug in the economy code.",
			country_instance, actual_military_spending_copy, actual_military_budget
		);
	}
	cash_stockpile -= actual_military_spending;

	const fixed_point_t actual_social_spending_copy = actual_pensions_spending.load() + actual_unemployment_subsidies_spending.load();
	if (OV_unlikely(actual_social_spending_copy > actual_social_budget)) {
		spdlog::error_s(
			"Country {} has overspend on pensions and/or unemployment subsidies. "
			"Spending {} on pensions and {} on unemployment subsidies instead of the total allocated {}. "
			"This indicates a severe bug in the economy code.",
			country_instance, actual_pensions_spending.load(), actual_unemployment_subsidies_spending.load(), actual_social_budget
		);
	}
	cash_stockpile -= actual_pensions_spending;
	cash_stockpile -= actual_unemployment_subsidies_spending;

	const fixed_point_t actual_import_subsidies_spending_copy = actual_import_subsidies_spending.load();
	if (OV_unlikely(actual_import_subsidies_spending_copy > actual_import_subsidies_budget)) {
		spdlog::error_s(
			"Country {} has overspend on import subsidies. Spending {} instead of the allocated {}. This indicates a severe bug in the economy code.",
			country_instance, actual_import_subsidies_spending_copy, actual_import_subsidies_budget
		);
	}
	cash_stockpile -= actual_import_subsidies_spending;

	const fixed_point_t cash_stockpile_copy = cash_stockpile.load();
	if (OV_unlikely(cash_stockpile_copy < 0)) {
		spdlog::error_s(
			"Country {} has overspend resulting in a cash stockpile of {}. This indicates a severe bug in the economy code.",
			country_instance, cash_stockpile_copy
		);
	}

	cash_stockpile += actual_tariff_income;

	const fixed_point_t gold_income_value = country_defines.get_gold_to_cash_rate() * total_gold_production;;
	gold_income.set(gold_income_value);
	cash_stockpile += gold_income_value;
	const fixed_point_t yesterdays_balance = cash_stockpile - cash_stockpile_start_of_tick;
	balance_history.push_back(yesterdays_balance);
}

void CountryEconomy::report_pop_income_tax(PopType const& pop_type, const fixed_point_t gross_income, const fixed_point_t paid_as_tax) {
	const std::lock_guard<std::mutex> lock_guard { taxable_income_mutex };
	taxable_income_by_pop_type.at(pop_type) += gross_income;
	cash_stockpile += paid_as_tax;
}

void CountryEconomy::report_pop_need_consumption(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.need_consumption_per_pop_type[&pop_type] += quantity;
}
void CountryEconomy::report_pop_need_demand(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.pop_demand += quantity;
}
void CountryEconomy::report_input_consumption(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.input_consumption_per_production_type[&production_type] += quantity;
}
void CountryEconomy::report_input_demand(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	if (production_type.get_template_type() == ProductionType::template_type_t::ARTISAN) {
		switch (game_rules_manager.get_artisanal_input_demand_category()) {
			case demand_category::FactoryNeeds: break;
			case demand_category::PopNeeds: {
				good_data_t& good_data = get_good_data(good);
				const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
				good_data.pop_demand += quantity;
				return;
			}
			default: return; //demand_category::None
		}
	}

	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.factory_demand += quantity;
}
void CountryEconomy::report_output(ProductionType const& production_type, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(production_type.get_output_good());
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.production_per_production_type[&production_type] += quantity;
}

void CountryEconomy::request_salaries_and_welfare_and_import_subsidies(Pop& pop) {
	PopType const& pop_type = *pop.get_type();
	const pop_size_t pop_size = pop.get_size();
	SharedPopTypeValues const& pop_type_values = shared_country_values.get_shared_pop_type_values(pop_type);

	if (actual_administration_budget > 0) {
		const fixed_point_t administration_salary = fixed_point_t::mul_div(
			pop_size * administration_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_administration_budget,
			projected_administration_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (administration_salary > 0) {
			pop.add_government_salary_administration(administration_salary);
			actual_administration_spending += administration_salary;
		}
	}

	if (actual_education_budget > 0) {
		const fixed_point_t education_salary = fixed_point_t::mul_div(
			pop_size * education_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_education_budget,
			projected_education_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (education_salary > 0) {
			pop.add_government_salary_education(education_salary);
			actual_education_spending += education_salary;
		}
	}

	if (actual_military_budget > 0) {
		const fixed_point_t military_salary = fixed_point_t::mul_div(
			pop_size * military_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_military_budget,
			projected_military_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (military_salary > 0) {
			pop.add_government_salary_military(military_salary);
			actual_military_spending += military_salary;
		}
	}

	if (actual_social_budget > 0) {
		const fixed_point_t pension_income = fixed_point_t::mul_div(
			pop_size * calculate_pensions_base(pop_type),
			actual_social_budget,
			projected_social_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (pension_income > 0) {
			pop.add_pensions(pension_income);
			actual_pensions_spending += pension_income;
		}

		const fixed_point_t unemployment_subsidies = fixed_point_t::mul_div(
			pop.get_unemployed() * calculate_unemployment_subsidies_base(pop_type),
			actual_social_budget,
			projected_social_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (unemployment_subsidies > 0) {
			pop.add_unemployment_subsidies(unemployment_subsidies);
			actual_unemployment_subsidies_spending += unemployment_subsidies;
		}
	}

	if (actual_import_subsidies_budget > 0) {
		const fixed_point_t import_subsidies = fixed_point_t::mul_div(
			effective_tariff_rate.get_untracked() // < 0
				* pop.get_yesterdays_import_value().get_copy_of_value(),
			actual_import_subsidies_budget, // < 0
			projected_import_subsidies.get_untracked() // > 0
		); //effective_tariff_rate * actual_net_tariffs cancel out the negative
		pop.add_import_subsidies(import_subsidies);
		actual_import_subsidies_spending += import_subsidies;
	}
}

fixed_point_t CountryEconomy::apply_tariff(const fixed_point_t money_spent_on_imports) {
	const fixed_point_t effective_tariff_rate_value = effective_tariff_rate.get_untracked();
	if (effective_tariff_rate_value <= 0) {
		return 0;
	}

	const fixed_point_t tariff = effective_tariff_rate_value * money_spent_on_imports;
	actual_tariff_income += tariff;
	return tariff;
}

void CountryEconomy::after_buy(void* actor, BuyResult const& buy_result) {
	const fixed_point_t quantity_bought = buy_result.quantity_bought;

	if (quantity_bought <= 0) {
		return;
	}

	CountryEconomy& country = *static_cast<CountryEconomy*>(actor);
	good_data_t& good_data = country.goods_data.at_index(buy_result.good_definition.get_index());
	const fixed_point_t money_spent = buy_result.money_spent_total;
	country.cash_stockpile -= money_spent;
	country.actual_national_stockpile_spending += money_spent;
	good_data.stockpile_amount += quantity_bought;
	good_data.stockpile_change_yesterday += quantity_bought;
	good_data.quantity_traded_yesterday = quantity_bought;
	good_data.money_traded_yesterday = -money_spent;
}

void CountryEconomy::after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) {
	const fixed_point_t quantity_sold = sell_result.quantity_sold;

	if (quantity_sold <= 0) {
		return;
	}

	CountryEconomy& country = *static_cast<CountryEconomy*>(actor);
	good_data_t& good_data = country.goods_data.at_index(sell_result.good_definition.get_index());
	const fixed_point_t money_gained = sell_result.money_gained;
	country.cash_stockpile += money_gained;
	country.actual_national_stockpile_income += money_gained;
	good_data.stockpile_amount -= quantity_sold;
	good_data.stockpile_change_yesterday -= quantity_sold;
	good_data.quantity_traded_yesterday = -quantity_sold;
	good_data.money_traded_yesterday = money_gained;
}

void CountryEconomy::manage_national_stockpile(
	CountryInstance const& country_instance,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	utility::forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_COUNTRY_BUDGET_TICK
	> reusable_vectors,
	memory::vector<size_t>& reusable_index_vector,
	const fixed_point_t available_funds
) {
	IndexedFlatMap<GoodDefinition, char>& wants_more_mask = reusable_goods_mask;
	const size_t mask_size = wants_more_mask.get_keys().size();
	memory::vector<fixed_point_t>& max_quantity_to_buy_per_good = reusable_vectors[0];
	max_quantity_to_buy_per_good.resize(mask_size, 0);
	memory::vector<fixed_point_t>& max_costs_per_good = reusable_vectors[1];
	max_costs_per_good.resize(mask_size, 0);
	memory::vector<fixed_point_t>& weights = reusable_vectors[2];
	weights.resize(mask_size, 0);
	memory::vector<size_t>& good_indices_to_buy = reusable_index_vector;
	fixed_point_t weights_sum = 0;

	for (auto const& [good_instance, good_data] : goods_data) {
		good_data.clear_daily_recorded_data();
		const size_t index = good_instance.get_index();
		if (good_data.is_selling) {
			const fixed_point_t quantity_to_sell = good_data.stockpile_amount - good_data.stockpile_cutoff;
			if (quantity_to_sell <= 0) {
				continue;
			}
			market_instance.place_market_sell_order(
				{
					good_instance.get_good_definition(),
					&country_instance,
					quantity_to_sell,
					this,
					after_sell,
				},
				reusable_vectors[3] //temporarily used here and later used as money_to_spend_per_good
			);
		} else {
			const fixed_point_t max_quantity_to_buy = good_data.stockpile_cutoff - good_data.stockpile_amount;
			if (max_quantity_to_buy <= 0 || available_funds <= 0) {
				continue;
			}

			good_indices_to_buy.push_back(index);
			max_quantity_to_buy_per_good[index] = max_quantity_to_buy;

			const fixed_point_t max_money_to_spend = max_costs_per_good[index] = market_instance.get_max_money_to_allocate_to_buy_quantity(
				good_instance.get_good_definition(),
				max_quantity_to_buy
			);
			wants_more_mask.set(good_instance.get_good_definition(), true);
			const fixed_point_t weight = weights[index] = fixed_point_t::usable_max / max_money_to_spend;
			weights_sum += weight;
		}
	}

	if (weights_sum > 0) {
		memory::vector<fixed_point_t>& money_to_spend_per_good = reusable_vectors[3];
		money_to_spend_per_good.resize(mask_size, 0);
		fixed_point_t cash_left_to_spend_draft = available_funds;
		bool needs_redistribution = true;
		while (needs_redistribution) {
			needs_redistribution = false;
			for (const size_t good_index : good_indices_to_buy) {
				char& wants_more = wants_more_mask.at_index(good_index);
				if (!wants_more) {
					continue;
				}

				const fixed_point_t weight = weights[good_index];
				const fixed_point_t max_costs = max_costs_per_good[good_index];
				
				fixed_point_t cash_available_for_good = fixed_point_t::mul_div(
					cash_left_to_spend_draft,
					weight,
					weights_sum
				);
				
				if (cash_available_for_good >= max_costs) {
					cash_left_to_spend_draft -= max_costs;
					money_to_spend_per_good[good_index] = max_costs;
					weights_sum -= weight;
					wants_more = false;
					needs_redistribution = weights_sum > 0;
					break;
				}

				GoodInstance const& good_instance = goods_data.get_keys()[good_index];
				GoodDefinition const& good_definition = good_instance.get_good_definition();
				const fixed_point_t max_possible_quantity_bought = cash_available_for_good / market_instance.get_min_next_price(good_definition);
				if (max_possible_quantity_bought < fixed_point_t::epsilon) {
					money_to_spend_per_good[good_index] = 0;
				} else {
					money_to_spend_per_good[good_index] = cash_available_for_good;
				}
			}
		}
		
		for (const size_t good_index : good_indices_to_buy) {
			const fixed_point_t max_quantity_to_buy = max_quantity_to_buy_per_good[good_index];
			const fixed_point_t money_to_spend = money_to_spend_per_good[good_index];
			if (money_to_spend <= 0) {
				continue;
			}

			GoodInstance const& good_instance = goods_data.get_keys()[good_index];
			GoodDefinition const& good_definition = good_instance.get_good_definition();
			market_instance.place_buy_up_to_order(
				{
					good_definition,
					&country_instance,
					max_quantity_to_buy,
					money_to_spend,
					this,
					after_buy
				}
			);
		}
	}

	reusable_goods_mask.fill(0);
	for (auto& reusable_vector : reusable_vectors) {
		reusable_vector.clear();
	}
	reusable_index_vector.clear();
}

fixed_point_t CountryEconomy::calculate_pensions_base(PopType const& pop_type) {
	return get_modifier_effect_value(*modifier_effect_cache.get_pension_level())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}
fixed_point_t CountryEconomy::calculate_unemployment_subsidies_base(PopType const& pop_type) {
	return get_modifier_effect_value(*modifier_effect_cache.get_unemployment_benefit())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}
fixed_point_t CountryEconomy::calculate_minimum_wage_base(PopType const& pop_type) {
	if (pop_type.get_is_slave()) {
		return 0;
	}
	return get_modifier_effect_value(*modifier_effect_cache.get_minimum_wage())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}