#include "ArtisanalProducer.hpp"

#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/pop/Pop.hpp"

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

void ArtisanalProducer::artisan_tick(Pop& pop) {
	CountryInstance* country_to_report_economy_nullable = pop.get_location()->get_country_to_report_economy();
	max_quantity_to_buy_per_good.clear();

	//TODO get from pool instead of create & destroy each time
	GoodDefinition::good_definition_map_t goods_to_buy_and_max_price { };
	GoodDefinition::good_definition_map_t demand { };

	//throughput scalar, the minimum of stockpile / desired_quantity
	fixed_point_t inputs_bought_fraction = fixed_point_t::_1(),
		inputs_bought_numerator= fixed_point_t::_1(),
		inputs_bought_denominator= fixed_point_t::_1();
	for (auto const& [input_good_ptr, base_desired_quantity] : production_type.get_input_goods()) {
		const fixed_point_t desired_quantity = demand[input_good_ptr] = base_desired_quantity * pop.get_size() / production_type.get_base_workforce_size();
		if (desired_quantity == fixed_point_t::_0()) {
			continue;
		}

		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_input_demand(
				production_type,
				*input_good_ptr,
				desired_quantity
			);
		}

		const fixed_point_t good_bought_fraction = stockpile[input_good_ptr] / desired_quantity;
		if (good_bought_fraction < inputs_bought_fraction) {
			inputs_bought_fraction = good_bought_fraction;
			inputs_bought_numerator = stockpile[input_good_ptr];
			inputs_bought_denominator = desired_quantity;
		}
		
		goods_to_buy_and_max_price[input_good_ptr] = pop.get_market_instance().get_max_next_price(*input_good_ptr);
	}

	//Produce output
	fixed_point_t produce_left_to_sell = current_production = production_type.get_base_output_quantity()
		* inputs_bought_numerator * pop.get_size()
		/ (inputs_bought_denominator * production_type.get_base_workforce_size());

	if (current_production > fixed_point_t::_0()) {
		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_output(production_type, current_production);
		}
	}

	if (inputs_bought_fraction > fixed_point_t::_0()) {
		for (auto const& [input_good_ptr, base_desired_quantity] : production_type.get_input_goods()) {
			const fixed_point_t desired_quantity = demand[input_good_ptr];
			fixed_point_t& good_stockpile = stockpile[input_good_ptr];

			//Consume input good
			fixed_point_t consumed_quantity = desired_quantity * inputs_bought_numerator / inputs_bought_denominator;
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
					produce_left_to_sell = fixed_point_t::_0();
				} else {
					produce_left_to_sell -= consumed_quantity;
					consumed_quantity = fixed_point_t::_0();
				}
			}

			good_stockpile = std::max(
				fixed_point_t::_0(),
				good_stockpile - consumed_quantity
			);

			if (good_stockpile >= desired_quantity) {
				goods_to_buy_and_max_price.erase(input_good_ptr);
			}
		}
	}

	const fixed_point_t total_cash_to_spend = pop.get_cash();
	if (total_cash_to_spend > fixed_point_t::_0() && !goods_to_buy_and_max_price.empty()) {
		//Figure out the optimal amount of goods to buy based on their price, stockpiled quantiy & demand
		fixed_point_t max_possible_satisfaction_numerator= fixed_point_t::_1(),
			max_possible_satisfaction_denominator= fixed_point_t::_1();

		bool at_or_below_optimum = false;
		while (!at_or_below_optimum) {
			at_or_below_optimum = true;
			fixed_point_t total_demand_value = fixed_point_t::_0();
			fixed_point_t total_stockpile_value = fixed_point_t::_0();
			for (auto const& [input_good_ptr, max_price] : goods_to_buy_and_max_price) {
				total_demand_value += max_price * demand[input_good_ptr];
				total_stockpile_value += max_price * stockpile[input_good_ptr];
			}

			if ( total_demand_value == fixed_point_t::_0()) {
				max_possible_satisfaction_numerator = fixed_point_t::_1();
				max_possible_satisfaction_denominator = fixed_point_t::_1();
			} else {
				max_possible_satisfaction_numerator = total_stockpile_value + total_cash_to_spend;
				max_possible_satisfaction_denominator = total_demand_value;
				if (max_possible_satisfaction_numerator > max_possible_satisfaction_denominator) {
					max_possible_satisfaction_numerator = fixed_point_t::_1();
					max_possible_satisfaction_denominator = fixed_point_t::_1();
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
				const fixed_point_t optimal_quantity = demand[input_good_ptr] * max_possible_satisfaction_numerator / max_possible_satisfaction_denominator;
				if (stockpile[input_good_ptr] < optimal_quantity) {
					return false;
				}
				at_or_below_optimum = false;
				return true;
			});
		}

		//Place buy orders for each input
		for (auto const& [input_good_ptr, max_price] : goods_to_buy_and_max_price) {
			const fixed_point_t good_demand = demand[input_good_ptr];
			fixed_point_t& good_stockpile = stockpile[input_good_ptr];
			const fixed_point_t optimal_quantity = good_demand * max_possible_satisfaction_numerator / max_possible_satisfaction_denominator;
			const fixed_point_t max_quantity_to_buy = good_demand - good_stockpile;
			if (max_quantity_to_buy > fixed_point_t::_0()) {
				const fixed_point_t money_to_spend = optimal_quantity * max_price;
				max_quantity_to_buy_per_good[input_good_ptr] = max_quantity_to_buy;
				pop.artisanal_buy(*input_good_ptr, max_quantity_to_buy, money_to_spend);
			}
		}
	}

	pop.artisanal_sell(produce_left_to_sell);
}

fixed_point_t ArtisanalProducer::add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity) {
	if (OV_unlikely(quantity < fixed_point_t::_0())) {
		Logger::error("Attempted to add negative quantity ",quantity," of ", good.get_identifier(), " to stockpile.");
		return fixed_point_t::_0();
	}

	fixed_point_t& max_quantity_to_buy = max_quantity_to_buy_per_good[&good];
	const fixed_point_t quantity_added_to_stockpile = std::min(quantity, max_quantity_to_buy);
	stockpile[&good] += quantity_added_to_stockpile;
	max_quantity_to_buy -= quantity_added_to_stockpile;
	return quantity_added_to_stockpile;
}