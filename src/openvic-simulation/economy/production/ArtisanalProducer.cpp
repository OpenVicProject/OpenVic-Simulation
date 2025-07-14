#include "ArtisanalProducer.hpp"

#include <cstddef>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;

ArtisanalProducer::ArtisanalProducer(
	ModifierEffectCache const& new_modifier_effect_cache,
	GoodDefinition::good_definition_map_t&& new_stockpile,
	ProductionType const& new_production_type,
	fixed_point_t new_current_production
) : modifier_effect_cache { new_modifier_effect_cache },
	stockpile { std::move(new_stockpile) },
	production_type { new_production_type },
	current_production { new_current_production }
	{
		max_quantity_to_buy_per_good.reserve(new_production_type.get_input_goods().size());
	}

void ArtisanalProducer::artisan_tick(
	Pop& pop,
	const fixed_point_t max_cost_multiplier,
	IndexedMap<GoodDefinition, char>& reusable_goods_mask,
	memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
	memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
	memory::vector<fixed_point_t>& reusable_map_0,
	memory::vector<fixed_point_t>& reusable_map_1
) {
	CountryInstance* country_to_report_economy_nullable = pop.get_location()->get_country_to_report_economy();
	max_quantity_to_buy_per_good.clear();
	GoodDefinition::good_definition_map_t const& input_goods = production_type.get_input_goods();
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
		if (desired_quantity == fixed_point_t::_0) {
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
		reusable_goods_mask[input_good] = true;
		distinct_goods_to_buy++;
	}

	//Produce output
	fixed_point_t produce_left_to_sell = current_production = production_type.get_base_output_quantity()
		* inputs_bought_numerator * pop.get_size()
		/ (inputs_bought_denominator * production_type.get_base_workforce_size());

	if (current_production > fixed_point_t::_0) {
		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_output(production_type, current_production);
		}
	}

	if (inputs_bought_fraction > fixed_point_t::_0) {
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
				reusable_goods_mask[input_good] = false;
				distinct_goods_to_buy--;
			}
		}
	}

	//executed once per pop while nothing else uses it.
	const fixed_point_t total_cash_to_spend = pop.get_cash().get_copy_of_value() / max_cost_multiplier;
	MarketInstance const& market_instance = pop.get_market_instance();

	if (total_cash_to_spend > fixed_point_t::_0 && distinct_goods_to_buy > 0) {
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
				if (!reusable_goods_mask[input_good]) {
					continue;
				}
				const ptrdiff_t i = it - input_goods.begin();
				const fixed_point_t max_price = max_price_per_input[i];
				total_demand_value += max_price * demand_per_input[i];
				total_stockpile_value += max_price * stockpile[&input_good];
			}

			if ( total_demand_value == fixed_point_t::_0) {
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
				char& mask = reusable_goods_mask[input_good];
				if (!mask) {
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
					mask = false;
					distinct_goods_to_buy--;
				}				
			}
		}

		fixed_point_t debug_cash_left = total_cash_to_spend;
		//Place buy orders for each input
		for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
			GoodDefinition const& input_good = *it.key();
			if (!reusable_goods_mask[input_good]) {
				continue;
			}
			const ptrdiff_t index_in_input_goods = it - input_goods.begin();
			const fixed_point_t good_demand = demand_per_input[index_in_input_goods];
			fixed_point_t& good_stockpile = stockpile[&input_good];
			const fixed_point_t max_quantity_to_buy = good_demand - good_stockpile;
			if (max_quantity_to_buy > fixed_point_t::_0) {
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

		if (OV_unlikely(debug_cash_left < fixed_point_t::_0)) {
			Logger::error("Artisan allocated more cash than the pop has. debug_cash_left: ", debug_cash_left);
		}
	}

	pop.report_artisanal_produce(produce_left_to_sell);
	reusable_map_0.clear();
	reusable_map_1.clear();
	reusable_goods_mask.clear();
}

fixed_point_t ArtisanalProducer::add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity) {
	if (OV_unlikely(quantity < fixed_point_t::_0)) {
		Logger::error("Attempted to add negative quantity ",quantity," of ", good.get_identifier(), " to stockpile.");
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