#include "ArtisanalProducer.hpp"
#include <cstdint>

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
	GoodDefinition::good_definition_map_t& pop_max_quantity_to_buy_per_good,
	GoodDefinition::good_definition_map_t& pop_money_to_spend_per_good,
	GoodDefinition::good_definition_map_t& reusable_map_0,
	GoodDefinition::good_definition_map_t& reusable_map_1
) {
	CountryInstance* country_to_report_economy_nullable = pop.get_location()->get_country_to_report_economy();
	max_quantity_to_buy_per_good.clear();
	GoodDefinition::good_definition_map_t& goods_to_buy_and_max_price = reusable_map_0;
	GoodDefinition::good_definition_map_t& demand = reusable_map_1;

	//throughput scalar, the minimum of stockpile / desired_quantity
	fixed_point_t inputs_bought_fraction = 1,
		inputs_bought_numerator= 1,
		inputs_bought_denominator= 1;
	for (auto const& [input_good_ptr, base_desired_quantity] : production_type.get_input_goods()) {
		const fixed_point_t desired_quantity = demand[input_good_ptr] = base_desired_quantity * pop.get_size() / production_type.get_base_workforce_size();
		if (desired_quantity == fixed_point_t::_0) {
			continue;
		}

		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_input_demand(
				production_type,
				*input_good_ptr,
				desired_quantity
			);
		}

		const fixed_point_t stockpiled_quantity = stockpile[input_good_ptr];
		const fixed_point_t good_bought_fraction = stockpiled_quantity / desired_quantity;
		if (good_bought_fraction < inputs_bought_fraction) {
			inputs_bought_fraction = good_bought_fraction;
			inputs_bought_numerator = stockpiled_quantity;
			inputs_bought_denominator = desired_quantity;
		}

		goods_to_buy_and_max_price[input_good_ptr] = pop.get_market_instance().get_max_next_price(*input_good_ptr);
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
		for (auto const& [input_good_ptr, base_desired_quantity] : production_type.get_input_goods()) {
			const fixed_point_t desired_quantity = demand[input_good_ptr];
			fixed_point_t& good_stockpile = stockpile[input_good_ptr];

			//Consume input good
			fixed_point_t consumed_quantity = fixed_point_t::mul_div(
				desired_quantity,
				inputs_bought_numerator,
				inputs_bought_denominator
			);
			if (country_to_report_economy_nullable != nullptr) {
				country_to_report_economy_nullable->report_input_consumption(
					production_type,
					*input_good_ptr,
					consumed_quantity
				);
			}
			if (input_good_ptr == &production_type.get_output_good()) {
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
				goods_to_buy_and_max_price.erase(input_good_ptr);
			}
		}
	}

	//executed once per pop while nothing else uses it.
	const fixed_point_t total_cash_to_spend = pop.get_cash().get_copy_of_value() / max_cost_multiplier;
	MarketInstance const& market_instance = pop.get_market_instance();

	if (total_cash_to_spend > fixed_point_t::_0 && !goods_to_buy_and_max_price.empty()) {
		//Figure out the optimal amount of goods to buy based on their price, stockpiled quantiy & demand
		fixed_point_t max_possible_satisfaction_numerator= 1,
			max_possible_satisfaction_denominator= 1;

		bool at_or_below_optimum = false;
		while (!at_or_below_optimum) {
			at_or_below_optimum = true;
			fixed_point_t total_demand_value = 0;
			fixed_point_t total_stockpile_value = 0;
			for (auto const& [input_good_ptr, max_price] : goods_to_buy_and_max_price) {
				total_demand_value += max_price * demand[input_good_ptr];
				total_stockpile_value += max_price * stockpile[input_good_ptr];
			}

			if ( total_demand_value == fixed_point_t::_0) {
				max_possible_satisfaction_numerator = 1;
				max_possible_satisfaction_denominator = 1;
			} else {
				//epsilon is the minimum costs for a trade.
				const fixed_point_t flat_costs = fixed_point_t::epsilon * static_cast<int32_t>(goods_to_buy_and_max_price.size());
				max_possible_satisfaction_numerator = total_stockpile_value + total_cash_to_spend - flat_costs;
				max_possible_satisfaction_denominator = total_demand_value + flat_costs;
				if (max_possible_satisfaction_numerator >= max_possible_satisfaction_denominator) {
					max_possible_satisfaction_numerator = 1;
					max_possible_satisfaction_denominator = 1;
				}
			}

			erase_if(goods_to_buy_and_max_price, [
				this,
				&demand,
				&at_or_below_optimum,
				max_possible_satisfaction_numerator,
				max_possible_satisfaction_denominator
			](auto const& pair)->bool {
				GoodDefinition const* const input_good_ptr = pair.first;
				const fixed_point_t optimal_quantity = fixed_point_t::mul_div(
					demand[input_good_ptr],
					max_possible_satisfaction_numerator,
					max_possible_satisfaction_denominator
				);
				if (stockpile[input_good_ptr] < optimal_quantity) {
					return false;
				}
				at_or_below_optimum = false;
				return true;
			});
		}

		fixed_point_t debug_cash_left = total_cash_to_spend;
		//Place buy orders for each input
		for (auto const& [input_good_ptr, _] : goods_to_buy_and_max_price) {
			const fixed_point_t good_demand = demand[input_good_ptr];
			fixed_point_t& good_stockpile = stockpile[input_good_ptr];
			const fixed_point_t max_quantity_to_buy = good_demand - good_stockpile;
			if (max_quantity_to_buy > fixed_point_t::_0) {
				const fixed_point_t optimal_quantity = fixed_point_t::mul_div(
					good_demand,
					max_possible_satisfaction_numerator,
					max_possible_satisfaction_denominator
				);
				const fixed_point_t money_to_spend = market_instance.get_max_money_to_allocate_to_buy_quantity(
					*input_good_ptr,
					optimal_quantity - good_stockpile
				);
				max_quantity_to_buy_per_good[input_good_ptr] = max_quantity_to_buy;
				pop.allocate_cash_for_artisanal_spending(money_to_spend);
				pop_max_quantity_to_buy_per_good[input_good_ptr] += max_quantity_to_buy;
				pop_money_to_spend_per_good[input_good_ptr] += money_to_spend;
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