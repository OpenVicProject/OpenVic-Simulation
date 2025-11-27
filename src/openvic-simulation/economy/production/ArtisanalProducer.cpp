#include "ArtisanalProducer.hpp"
#include "ArtisanalProducerDeps.hpp"

#include <cstddef>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/EconomyDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/RandomGenerator.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/utility/WeightedSampling.hpp"

using namespace OpenVic;

ArtisanalProducer::ArtisanalProducer(ArtisanalProducerDeps const& artisanal_producer_deps) : ArtisanalProducer(
	artisanal_producer_deps,
	IndexedFlatMap<GoodDefinition, fixed_point_t> { artisanal_producer_deps.good_keys },
	nullptr,
	nullptr,
	fixed_point_t::_0
) { };

ArtisanalProducer::ArtisanalProducer(
	ArtisanalProducerDeps const& artisanal_producer_deps,
	IndexedFlatMap<GoodDefinition, fixed_point_t>&& new_stockpile,
	ProductionType const* const new_production_type,
	GoodDefinition const* const new_last_produced_good,
	fixed_point_t new_current_production
) : economy_defines { artisanal_producer_deps.economy_defines },
	modifier_effect_cache { artisanal_producer_deps.modifier_effect_cache },
	stockpile { std::move(new_stockpile) },
	production_type_nullable { nullptr },
	last_produced_good { new_last_produced_good },
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

void ArtisanalProducer::artisan_tick_handler::calculate_inputs(
	IndexedFlatMap<GoodDefinition, fixed_point_t> const& stockpile,
	const bool should_report_input_demand
) {
	fixed_point_map_t<GoodDefinition const*> const& input_goods = production_type.get_input_goods();

	//throughput scalar, the minimum of stockpile / base_desired_quantity
	//inputs_bought_fraction uses base_desired_quantity as population size is cancelled in the production and input calculations.
	const pop_size_t pop_size = pop.get_size();
	fixed_point_t inputs_bought_numerator = pop_size,
		inputs_bought_denominator = production_type.base_workforce_size,
		inputs_bought_fraction_v = inputs_bought_numerator / inputs_bought_denominator;

	distinct_goods_to_buy = 0;
	wants_more_mask.fill(false);

	for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
		GoodDefinition const& input_good = *it.key();
		const fixed_point_t base_desired_quantity = it.value();
		const ptrdiff_t i = it - input_goods.begin();
		const fixed_point_t desired_quantity = demand_per_input[i] = base_desired_quantity * pop_size / production_type.base_workforce_size;
		if (desired_quantity == 0) {
			continue;
		}

		if (should_report_input_demand && country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_input_demand(
				production_type,
				input_good,
				desired_quantity
			);
		}

		const fixed_point_t stockpiled_quantity = stockpile.at(input_good);
		const fixed_point_t good_bought_fraction = stockpiled_quantity / base_desired_quantity;
		if (good_bought_fraction < inputs_bought_fraction_v) {
			inputs_bought_fraction_v = good_bought_fraction;
			inputs_bought_numerator = stockpiled_quantity;
			inputs_bought_denominator = base_desired_quantity;
		}

		max_price_per_input[i] = market_instance.get_max_next_price(input_good);
		wants_more_mask.set(input_good, true);
		distinct_goods_to_buy++;
	}

	inputs_bought_fraction = Fraction {
		inputs_bought_numerator,
		inputs_bought_denominator
	};
}

void ArtisanalProducer::artisan_tick_handler::produce(
	fixed_point_t& costs_of_production,
	fixed_point_t& current_production,
	IndexedFlatMap<GoodDefinition, fixed_point_t>& stockpile
) {
	fixed_point_t produce_left_to_sell = current_production = production_type.base_output_quantity * inputs_bought_fraction;

	costs_of_production = 0;
	if (current_production == 0) {
		return;
	}

	if (country_to_report_economy_nullable != nullptr) {
		country_to_report_economy_nullable->report_output(production_type, current_production);
	}

	fixed_point_map_t<GoodDefinition const*> const& input_goods = production_type.get_input_goods();
	for (auto it = input_goods.begin(); it < input_goods.end(); it++) {
		GoodDefinition const& input_good = *it.key();
		const fixed_point_t base_desired_quantity = it.value();
		const ptrdiff_t i = it - input_goods.begin();
		fixed_point_t& good_stockpile = stockpile.at(input_good);

		//Consume input good
		fixed_point_t consumed_quantity = base_desired_quantity * inputs_bought_fraction;
		costs_of_production += consumed_quantity * market_instance.get_good_instance(input_good).get_price();

		if (country_to_report_economy_nullable != nullptr) {
			country_to_report_economy_nullable->report_input_consumption(
				production_type,
				input_good,
				consumed_quantity
			);
		}

		if (input_good == production_type.output_good) {
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

		if (good_stockpile >= demand_per_input[i]) {
			wants_more_mask.set(input_good, false);
			distinct_goods_to_buy--;
		}
	}

	stockpile.at(production_type.output_good) += produce_left_to_sell;
}

void ArtisanalProducer::artisan_tick_handler::allocate_money_for_inputs(
	fixed_point_map_t<GoodDefinition const*>& max_quantity_to_buy_per_good,
	memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
	memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
	IndexedFlatMap<GoodDefinition, fixed_point_t> const& stockpile,
	PopValuesFromProvince const& values_from_province
) {
	//executed once per pop while nothing else uses it.
	const fixed_point_t total_cash_to_spend = pop.get_cash().get_copy_of_value() / values_from_province.get_max_cost_multiplier();

	if (total_cash_to_spend <= 0 || distinct_goods_to_buy <= 0) {
		return;
	}

	//Figure out the optimal amount of goods to buy based on their price, stockpiled quantiy & demand
	fixed_point_t max_possible_satisfaction_numerator= 1,
		max_possible_satisfaction_denominator= 1;

	fixed_point_map_t<GoodDefinition const*> const& input_goods = production_type.get_input_goods();
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
			total_stockpile_value += max_price * stockpile.at(input_good);
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
			
			const fixed_point_t stockpiled_quantity = stockpile.at(input_good);
			if (stockpiled_quantity >= optimal_quantity) {
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
		const fixed_point_t stockpiled_quantity = stockpile.at(input_good);

		const fixed_point_t max_quantity_to_buy = good_demand - stockpiled_quantity;
		if (max_quantity_to_buy > 0) {
			const fixed_point_t optimal_quantity = fixed_point_t::mul_div(
				good_demand,
				max_possible_satisfaction_numerator,
				max_possible_satisfaction_denominator
			);
			const fixed_point_t money_to_spend = market_instance.get_max_money_to_allocate_to_buy_quantity(
				input_good,
				optimal_quantity - stockpiled_quantity
			);
			max_quantity_to_buy_per_good[&input_good] = max_quantity_to_buy;
			pop.allocate_cash_for_artisanal_spending(money_to_spend);
			const size_t index_in_all_goods = type_safe::get(input_good.index);
			pop_max_quantity_to_buy_per_good[index_in_all_goods] += max_quantity_to_buy;
			pop_money_to_spend_per_good[index_in_all_goods] += money_to_spend;
			debug_cash_left -= money_to_spend;
		}
	}

	if (OV_unlikely(debug_cash_left < 0)) {
		spdlog::error_s("Artisan allocated more cash than the pop has. debug_cash_left: {}", debug_cash_left);
	}
}

void ArtisanalProducer::artisan_tick(
	MarketInstance const& market_instance,
	Pop& pop,
	PopValuesFromProvince const& values_from_province,
	RandomU32& random_number_generator,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
	memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
	memory::vector<fixed_point_t>& reusable_map_0,
	memory::vector<fixed_point_t>& reusable_map_1,
	fixed_point_map_t<GoodDefinition const*>& goods_to_sell
) {
	CountryInstance* const country_to_report_economy_nullable = pop.get_location()->get_country_to_report_economy();
	max_quantity_to_buy_per_good.clear();
	IndexedFlatMap<GoodDefinition, char>& wants_more_mask = reusable_goods_mask;
	memory::vector<fixed_point_t>& max_price_per_input = reusable_map_0;
	memory::vector<fixed_point_t>& demand_per_input = reusable_map_1;

	ProductionType const* const old_production_type_ptr = production_type_nullable;

	if (values_from_province.game_rules_manager.get_should_artisans_discard_unsold_non_inputs()) {
		if (old_production_type_ptr == nullptr) {
			stockpile.fill(0);
		} else {
			fixed_point_map_t<GoodDefinition const*> const& input_goods = old_production_type_ptr->get_input_goods();
			for (auto [good, stockpiled_quantity] : stockpile) {
				if (!input_goods.contains(&good)) {
					stockpiled_quantity = 0;
				}
			}
		}
	}

	ProductionType const* new_production_type_ptr = pick_production_type(
		pop,
		values_from_province,
		random_number_generator
	);

	//if there is no valid production type, keep doing what we were doing, like in Vic2.
	if (new_production_type_ptr == nullptr) {
		new_production_type_ptr = old_production_type_ptr;
	}

	if (old_production_type_ptr != nullptr) {
		ProductionType const& old_production_type = *old_production_type_ptr;
		fixed_point_map_t<GoodDefinition const*> const& input_goods = old_production_type.get_input_goods();
		max_price_per_input.resize(input_goods.size(), 0);
		demand_per_input.resize(input_goods.size(), 0);

		artisan_tick_handler tick_handler {
			country_to_report_economy_nullable,
			demand_per_input,
			market_instance,
			max_price_per_input,
			pop,
			old_production_type,
			wants_more_mask
		};
		
		const bool should_report_input_demand = old_production_type_ptr == new_production_type_ptr;
		tick_handler.calculate_inputs(stockpile, should_report_input_demand);
		tick_handler.produce(costs_of_production, current_production, stockpile);
		last_produced_good = &old_production_type.output_good;
		if (old_production_type_ptr == new_production_type_ptr) {
			tick_handler.allocate_money_for_inputs(
				max_quantity_to_buy_per_good,
				pop_max_quantity_to_buy_per_good,
				pop_money_to_spend_per_good,
				stockpile,
				values_from_province
			);
		}
	}
	
	if (new_production_type_ptr != old_production_type_ptr) {
		set_production_type(new_production_type_ptr);
		ProductionType const& new_production_type = *new_production_type_ptr;

		fixed_point_map_t<GoodDefinition const*> const& input_goods = new_production_type.get_input_goods();
		max_price_per_input.resize(input_goods.size(), 0);
		demand_per_input.resize(input_goods.size(), 0);

		artisan_tick_handler tick_handler {
			country_to_report_economy_nullable,
			demand_per_input,
			market_instance,
			max_price_per_input,
			pop,
			new_production_type,
			wants_more_mask
		};
		
		constexpr bool should_report_input_demand = true;
		tick_handler.calculate_inputs(stockpile, should_report_input_demand);
		tick_handler.allocate_money_for_inputs(
			max_quantity_to_buy_per_good,
			pop_max_quantity_to_buy_per_good,
			pop_money_to_spend_per_good,
			stockpile,
			values_from_province
		);
	}

	for (auto const& [good, stockpiled_quantity] : stockpile) {
		if (production_type_nullable == nullptr || production_type_nullable->get_input_goods().contains(&good)) {
			continue;
		}

		goods_to_sell[&good] = stockpiled_quantity;
	}

	reusable_map_0.clear();
	reusable_map_1.clear();
	reusable_goods_mask.fill(0);
}

fixed_point_t ArtisanalProducer::add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity) {
	if (OV_unlikely(quantity < 0)) {
		spdlog::error_s(
			"Attempted to add negative quantity {} of {} to artisan stockpile.",
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
	stockpile.at(good) += quantity_added_to_stockpile;
	max_quantity_to_buy -= quantity_added_to_stockpile;
	return quantity_added_to_stockpile;
}

void ArtisanalProducer::subtract_from_stockpile(GoodDefinition const& good, const fixed_point_t sold_quantity) {
	if (OV_unlikely(sold_quantity < 0)) {
		spdlog::error_s(
			"Attempted to subtract negative quantity {} of {} to artisan stockpile.",
			sold_quantity, good
		);
		return;
	}

	fixed_point_t& stockpiled_quantity = stockpile.at(good);
	if (OV_unlikely(stockpiled_quantity < sold_quantity)) {
		spdlog::error_s(
			"Attempted to subtract more {} of {} from artisan stockpile than it has {}.",
			sold_quantity, good, stockpiled_quantity
		);

		stockpiled_quantity = 0;
		return;
	}

	stockpiled_quantity -= sold_quantity;
}

std::optional<fixed_point_t> ArtisanalProducer::estimate_production_type_score(
	GoodInstanceManager const& good_instance_manager,
	ProductionType const& production_type,
	ProvinceInstance& location,
	const fixed_point_t max_cost_multiplier
) {
	if (production_type.template_type != ProductionType::template_type_t::ARTISAN) {
		return std::nullopt;
	}

	if (!production_type.is_valid_for_artisan_in(location)) {
		return std::nullopt;
	}

	GoodInstance const& output_good = good_instance_manager.get_good_instance_by_definition(production_type.output_good);
	if (!output_good.get_is_available()) {
		return std::nullopt;
	}

	fixed_point_t estimated_costs = 0;
	for (auto const& [input_good, input_quantity] : production_type.get_input_goods()) {
		estimated_costs += input_quantity * good_instance_manager.get_good_instance_by_definition(*input_good).get_price();
	}
	estimated_costs *= max_cost_multiplier;

	const fixed_point_t estimated_revenue = production_type.base_output_quantity * output_good.get_price();
	return calculate_production_type_score(
		estimated_revenue,
		estimated_costs,
		production_type.base_workforce_size
	);
}

fixed_point_t ArtisanalProducer::calculate_production_type_score(
	const fixed_point_t revenue,
	const fixed_point_t costs,
	const pop_size_t workforce
) {
	if (costs >= revenue) {
		return 0;
	}

	//the score penalty at 0 margin
	constexpr fixed_point_t k = fixed_point_t::_0_50;
	static_assert(0 <= k);
	static_assert(k <= 1);

	return (
		k * fixed_point_t::mul_div(costs, costs, revenue)
		-(1+k)*costs
		+ revenue
	) * Pop::size_denominator / workforce; //factor out pop size without making values too small
}

ProductionType const* ArtisanalProducer::pick_production_type(
	Pop& pop,
	PopValuesFromProvince const& values_from_province,
	RandomU32& random_number_generator
) const {
	bool should_pick_new_production_type;
	const auto ranked_artisanal_production_types = values_from_province.get_ranked_artisanal_production_types();

	if (ranked_artisanal_production_types.empty()) {
		return production_type_nullable;
	}

	const fixed_point_t revenue = pop.get_artisanal_revenue();
	const fixed_point_t costs = costs_of_production;
	const fixed_point_t base_chance_to_switch_while_profitable = economy_defines.get_goods_focus_swap_chance();
	if (production_type_nullable == nullptr || (revenue <= costs)) {
		should_pick_new_production_type = true;
	} else if (base_chance_to_switch_while_profitable > 0) {
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

		//picked so the line hits 0 at relative_score = 2/3 and the area under the curve equals 1
		//2/3 was picked as being in the top 1/3 of production types makes it very unlikely you'll profit from switching.
		constexpr fixed_point_t slope = fixed_point_t{-9} / 2;
		constexpr fixed_point_t offset = 3;

		const fixed_point_t change_modifier_from_relative_score = std::max(fixed_point_t::_0, slope * relative_score + offset);
		const fixed_point_t switch_chance = base_chance_to_switch_while_profitable * change_modifier_from_relative_score;
		if (switch_chance >= 1) {
			should_pick_new_production_type = true;
		} else {
			constexpr fixed_point_t weights_sum = 1;
			const fixed_point_t keep_current_chance = weights_sum - switch_chance;
			const std::array<fixed_point_t, 2> weights { keep_current_chance, switch_chance };
			const size_t should_switch = sample_weighted_index(
				random_number_generator(),
				weights,
				weights_sum
			);
			should_pick_new_production_type = should_switch == 1;
		}
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

	const size_t sample_index = sample_weighted_index(
		random_number_generator(),
		weights,
		weights_sum
	);

	assert(sample_index >= 0 && sample_index < ranked_artisanal_production_types.size());
	return ranked_artisanal_production_types[sample_index].first;
}