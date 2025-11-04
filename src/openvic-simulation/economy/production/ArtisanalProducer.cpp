#include "ArtisanalProducer.hpp"

#include <cstddef>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

using namespace OpenVic;

ArtisanalProducer::ArtisanalProducer(
	ModifierEffectCache const& new_modifier_effect_cache,
	fixed_point_map_t<GoodDefinition const*>&& new_stockpile,
	ProductionType const* const new_production_type,
	fixed_point_t new_current_production
) : modifier_effect_cache { new_modifier_effect_cache },
	stockpile { std::move(new_stockpile) },
	production_type_nullable { nullptr },
	current_production { new_current_production }
	{
		set_production_type(new_production_type);
	}

void ArtisanalProducer::set_production_type(ProductionType const* const new_production_type) {
	if (production_type_nullable == new_production_type) {
		return;
	}

	production_type_nullable = new_production_type;
	if (production_type_nullable == nullptr) {
		return;
	}

	ProductionType const& production_type = *production_type_nullable;
	max_quantity_to_buy_per_good.clear();
	max_quantity_to_buy_per_good.reserve(production_type.get_input_goods().size());
}

void ArtisanalProducer::artisan_tick(
	Pop& pop,
	PopValuesFromProvince const& values_from_province,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
	memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
	memory::vector<fixed_point_t>& reusable_map_0,
	memory::vector<fixed_point_t>& reusable_map_1
) {
	ProductionType const* const old_production_type = production_type_nullable;
	set_production_type(pick_production_type(pop, values_from_province));
	if (production_type_nullable == nullptr) {
		return;
	}

	ProductionType const& production_type = *production_type_nullable;
	if (production_type_nullable != old_production_type) {
		//TODO sell stockpile no longer used
		stockpile.clear();
		for (auto const& [input_good, base_demand] : production_type.get_input_goods()) {
			stockpile[input_good] = base_demand * pop.get_size() / production_type.get_base_workforce_size();
		}
	}

	CountryInstance* country_to_report_economy_nullable = pop.get_location()->get_country_to_report_economy();
	max_quantity_to_buy_per_good.clear();
	IndexedFlatMap<GoodDefinition, char>& wants_more_mask = reusable_goods_mask;
	fixed_point_map_t<GoodDefinition const*> const& input_goods = production_type.get_input_goods();
	memory::vector<fixed_point_t>& max_price_per_input = reusable_map_0;
	max_price_per_input.resize(input_goods.size(), 0);
	memory::vector<fixed_point_t>& demand_per_input = reusable_map_1;
	demand_per_input.resize(input_goods.size(), 0);

	//throughput scalar, the minimum of stockpile / desired_quantity
	fixed_point_t inputs_bought_fraction = 1,
		inputs_bought_numerator= 1,
		inputs_bought_denominator= 1;

	size_t distinct_goods_to_buy = 0;
	
	for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
		GoodDefinition const& input_good = *it.key();
		const fixed_point_t base_desired_quantity = it.value();
		const ptrdiff_t i = it - input_goods.begin();
		const fixed_point_t desired_quantity = demand_per_input[i] = base_desired_quantity * pop.get_size() / production_type.get_base_workforce_size();
		if (desired_quantity == 0) {
			continue;
		}

		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_input_demand(
				production_type,
				input_good,
				desired_quantity
			);
		}

		const fixed_point_t stockpiled_quantity = stockpile[&input_good];
		const fixed_point_t good_bought_fraction = stockpiled_quantity / desired_quantity;
		if (good_bought_fraction < inputs_bought_fraction) {
			inputs_bought_fraction = good_bought_fraction;
			inputs_bought_numerator = stockpiled_quantity;
			inputs_bought_denominator = desired_quantity;
		}

		max_price_per_input[i] = pop.get_market_instance().get_max_next_price(input_good);
		wants_more_mask.set(input_good, true);
		distinct_goods_to_buy++;
	}

	//Produce output
	fixed_point_t produce_left_to_sell = current_production = production_type.get_base_output_quantity()
		* inputs_bought_numerator * pop.get_size()
		/ (inputs_bought_denominator * production_type.get_base_workforce_size());

	if (current_production > 0) {
		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_output(production_type, current_production);
		}
	}

	if (inputs_bought_fraction > 0) {
		for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
			GoodDefinition const& input_good = *it.key();
			const fixed_point_t base_desired_quantity = it.value();
			const ptrdiff_t i = it - input_goods.begin();
			const fixed_point_t desired_quantity = demand_per_input[i];
			fixed_point_t& good_stockpile = stockpile[&input_good];

			//Consume input good
			fixed_point_t consumed_quantity = fixed_point_t::mul_div(
				desired_quantity,
				inputs_bought_numerator,
				inputs_bought_denominator
			);
			if (country_to_report_economy_nullable != nullptr) {
				country_to_report_economy_nullable->report_input_consumption(
					production_type,
					input_good,
					consumed_quantity
				);
			}
			if (input_good == production_type.get_output_good()) {
				if (OV_unlikely(consumed_quantity > produce_left_to_sell)) {
					consumed_quantity -= produce_left_to_sell;
					produce_left_to_sell = 0;
				} else {
					produce_left_to_sell -= consumed_quantity;
					consumed_quantity = 0;
				}
			}

			good_stockpile = std::max(
				fixed_point_t::_0,
				good_stockpile - consumed_quantity
			);

			if (good_stockpile >= desired_quantity) {
				wants_more_mask.set(input_good, false);
				distinct_goods_to_buy--;
			}
		}
	}

	//executed once per pop while nothing else uses it.
	const fixed_point_t total_cash_to_spend = pop.get_cash().get_copy_of_value() / values_from_province.get_max_cost_multiplier();
	MarketInstance const& market_instance = pop.get_market_instance();

	if (total_cash_to_spend > 0 && distinct_goods_to_buy > 0) {
		//Figure out the optimal amount of goods to buy based on their price, stockpiled quantiy & demand
		fixed_point_t max_possible_satisfaction_numerator= 1,
			max_possible_satisfaction_denominator= 1;

		bool at_or_below_optimum = false;
		while (!at_or_below_optimum) {
			at_or_below_optimum = true;
			fixed_point_t total_demand_value = 0;
			fixed_point_t total_stockpile_value = 0;
			for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
				GoodDefinition const& input_good = *it.key();
				if (!wants_more_mask.at(input_good)) {
					continue;
				}
				const ptrdiff_t i = it - input_goods.begin();
				const fixed_point_t max_price = max_price_per_input[i];
				total_demand_value += max_price * demand_per_input[i];
				total_stockpile_value += max_price * stockpile[&input_good];
			}

			if ( total_demand_value == 0) {
				max_possible_satisfaction_numerator = 1;
				max_possible_satisfaction_denominator = 1;
			} else {
				//epsilon is the minimum costs for a trade.
				const fixed_point_t flat_costs = fixed_point_t::epsilon * distinct_goods_to_buy;
				max_possible_satisfaction_numerator = total_stockpile_value + total_cash_to_spend - flat_costs;
				max_possible_satisfaction_denominator = total_demand_value + flat_costs;
				if (max_possible_satisfaction_numerator >= max_possible_satisfaction_denominator) {
					max_possible_satisfaction_numerator = 1;
					max_possible_satisfaction_denominator = 1;
				}
			}

			for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
				GoodDefinition const& input_good = *it.key();
				char& wants_more = wants_more_mask.at(input_good);
				if (!wants_more) {
					continue;
				}
				const ptrdiff_t i = it - input_goods.begin();

				const fixed_point_t optimal_quantity = fixed_point_t::mul_div(
					demand_per_input[i],
					max_possible_satisfaction_numerator,
					max_possible_satisfaction_denominator
				);
				
				if (stockpile[&input_good] >= optimal_quantity) {
					at_or_below_optimum = false;
					wants_more = false;
					--distinct_goods_to_buy;
				}				
			}
		}

		fixed_point_t debug_cash_left = total_cash_to_spend;
		//Place buy orders for each input
		for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
			GoodDefinition const& input_good = *it.key();
			if (!wants_more_mask.at(input_good)) {
				continue;
			}
			const ptrdiff_t index_in_input_goods = it - input_goods.begin();
			const fixed_point_t good_demand = demand_per_input[index_in_input_goods];
			fixed_point_t& good_stockpile = stockpile[&input_good];
			const fixed_point_t max_quantity_to_buy = good_demand - good_stockpile;
			if (max_quantity_to_buy > 0) {
				const fixed_point_t optimal_quantity = fixed_point_t::mul_div(
					good_demand,
					max_possible_satisfaction_numerator,
					max_possible_satisfaction_denominator
				);
				const fixed_point_t money_to_spend = market_instance.get_max_money_to_allocate_to_buy_quantity(
					input_good,
					optimal_quantity - good_stockpile
				);
				max_quantity_to_buy_per_good[&input_good] = max_quantity_to_buy;
				pop.allocate_cash_for_artisanal_spending(money_to_spend);
				const size_t index_in_all_goods = input_good.get_index();
				pop_max_quantity_to_buy_per_good[index_in_all_goods] += max_quantity_to_buy;
				pop_money_to_spend_per_good[index_in_all_goods] += money_to_spend;
				debug_cash_left -= money_to_spend;
			}
		}

		if (OV_unlikely(debug_cash_left < 0)) {
			spdlog::error_s("Artisan allocated more cash than the pop has. debug_cash_left: {}", debug_cash_left);
		}
	}

	pop.report_artisanal_produce(produce_left_to_sell);
	reusable_map_0.clear();
	reusable_map_1.clear();
	reusable_goods_mask.fill(0);
}

fixed_point_t ArtisanalProducer::add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity) {
	if (OV_unlikely(quantity < 0)) {
		spdlog::error_s(
			"Attempted to add negative quantity {} of {} to stockpile.",
			quantity, good
		);
		return 0;
	}

	auto it = max_quantity_to_buy_per_good.find(&good);
	if (it == max_quantity_to_buy_per_good.end()) {
		return 0;
	}

	fixed_point_t& max_quantity_to_buy = it.value();
	const fixed_point_t quantity_added_to_stockpile = std::min(quantity, max_quantity_to_buy);
	stockpile.at(&good) += quantity_added_to_stockpile;
	max_quantity_to_buy -= quantity_added_to_stockpile;
	return quantity_added_to_stockpile;
}

std::optional<fixed_point_t> ArtisanalProducer::estimate_production_type_score(
	GoodInstanceManager const& good_instance_manager,
	ProductionType const& production_type,
	ProvinceInstance& location,
	const fixed_point_t max_cost_multiplier
) {
	if (production_type.get_template_type() != ProductionType::template_type_t::ARTISAN) {
		return std::nullopt;
	}

	if (!production_type.is_valid_for_artisan_in(location)) {
		return std::nullopt;
	}

	GoodInstance const& output_good = good_instance_manager.get_good_instance_by_definition(production_type.get_output_good());
	if (!output_good.get_is_available()) {
		return std::nullopt;
	}

	fixed_point_t estimated_costs = 0;
	for (auto const& [input_good, input_quantity] : production_type.get_input_goods()) {
		estimated_costs += input_quantity * good_instance_manager.get_good_instance_by_definition(*input_good).get_price();
	}
	estimated_costs *= max_cost_multiplier;

	const fixed_point_t estimated_revenue = production_type.get_base_output_quantity() * output_good.get_price();
	return calculate_production_type_score(
		estimated_revenue,
		estimated_costs,
		production_type.get_base_workforce_size()
	);
}

fixed_point_t ArtisanalProducer::calculate_production_type_score(
	const fixed_point_t revenue,
	const fixed_point_t costs,
	const pop_size_t workforce
) {
	constexpr fixed_point_t k = fixed_point_t::_0_50;
	return (
		k * fixed_point_t::mul_div(costs, costs, revenue)
		-(1+k)*costs
		+ revenue
	) * Pop::size_denominator / workforce; //factor out pop size without making values too small
}

ProductionType const* ArtisanalProducer::pick_production_type(
	Pop& pop,
	PopValuesFromProvince const& values_from_province
) const {
	bool should_pick_new_production_type;
	const auto ranked_artisanal_production_types = values_from_province.get_ranked_artisanal_production_types();

	if (ranked_artisanal_production_types.empty()) {
		return production_type_nullable;
	}

	const fixed_point_t revenue = pop.get_artisanal_income();
	const fixed_point_t costs = pop.get_artisan_inputs_expense();
	if (production_type_nullable == nullptr || (revenue <= costs)) {
		should_pick_new_production_type = true;
	} else {
		const fixed_point_t current_score = calculate_production_type_score(
			revenue,
			costs,		
			pop.get_size()
		);

		fixed_point_t relative_score = ranked_artisanal_production_types.empty() ? fixed_point_t::_1 : fixed_point_t::_0;
		for (auto it = ranked_artisanal_production_types.begin(); it < ranked_artisanal_production_types.end(); ++it) {
			auto const& [production_type, score_estimate] = *it;
			size_t i = it - ranked_artisanal_production_types.begin();
			if (current_score > score_estimate) {
				if (i == 0) {
					relative_score = fixed_point_t::_1;
				} else {
					const fixed_point_t previous_score_estimate = ranked_artisanal_production_types[i-1].second;
					relative_score = (
						(current_score - score_estimate)
						/ (previous_score_estimate - score_estimate)
						+ ranked_artisanal_production_types.size() - i
					) / (1 + ranked_artisanal_production_types.size());
				}
				break;
			}

			if (current_score == score_estimate) {
				relative_score = fixed_point_t::parse(ranked_artisanal_production_types.size() - i) / (1 + ranked_artisanal_production_types.size());
			}
		}

		//TODO decide based on score and randomness and defines.economy.GOODS_FOCUS_SWAP_CHANCE
		should_pick_new_production_type = relative_score < fixed_point_t::_0_50;
	}

	if (!should_pick_new_production_type) {
		return production_type_nullable;
	}
	
	fixed_point_t weights_sum = 0;
	memory::vector<fixed_point_t> weights {};
	weights.reserve(ranked_artisanal_production_types.size());
	for (auto const& [production_type, score_estimate] : ranked_artisanal_production_types) {
		//TODO calculate actual scores including availability of goods
		const fixed_point_t weight = score_estimate * score_estimate;
		weights.push_back(weight);
		weights_sum += weight;
	}

	//TODO randomly sample using weights
	return ranked_artisanal_production_types[0].first;
}