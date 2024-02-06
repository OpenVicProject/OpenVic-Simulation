#include "FactoryProducer.hpp"

using namespace OpenVic;

FactoryProducer::FactoryProducer(
	ProductionType const& new_production_type, const fixed_point_t new_size_multiplier,
	const fixed_point_t new_revenue_yesterday, const fixed_point_t new_output_quantity_yesterday,
	const fixed_point_t new_unsold_quantity_yesterday, ordered_map<Pop*, Pop::pop_size_t>&& new_employees,
	Good::good_map_t&& new_stockpile, fixed_point_t new_budget, const fixed_point_t new_balance_yesterday,
	const fixed_point_t new_received_investments_yesterday, const fixed_point_t new_market_spendings_yesterday,
	const fixed_point_t new_paychecks_yesterday, const uint32_t new_unprofitable_days, const uint32_t new_subsidised_days,
	const uint32_t new_days_without_input, const uint8_t new_hiring_priority, const uint8_t new_profit_history_current,
	daily_profit_history_t&& new_daily_profit_history
)
	: production_type { new_production_type }, size_multiplier { new_size_multiplier },
	  revenue_yesterday { new_revenue_yesterday }, output_quantity_yesterday { new_output_quantity_yesterday },
	  unsold_quantity_yesterday { new_unsold_quantity_yesterday }, employees { std::move(new_employees) },
	  stockpile { std::move(new_stockpile) }, budget { new_budget }, balance_yesterday { new_balance_yesterday },
	  received_investments_yesterday { new_received_investments_yesterday },
	  market_spendings_yesterday { new_market_spendings_yesterday }, paychecks_yesterday { new_paychecks_yesterday },
	  unprofitable_days { new_unprofitable_days }, subsidised_days { new_subsidised_days },
	  days_without_input { new_days_without_input }, hiring_priority { new_hiring_priority },
	  profit_history_current { new_profit_history_current }, daily_profit_history { std::move(new_daily_profit_history) } {}
FactoryProducer::FactoryProducer(ProductionType const& new_production_type, const fixed_point_t new_size_multiplier)
	: FactoryProducer { new_production_type, new_size_multiplier, 0, 0, 0, {}, {}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {} } {}

fixed_point_t FactoryProducer::get_profitability_yesterday() const {
	return daily_profit_history[profit_history_current];
}

fixed_point_t FactoryProducer::get_average_profitability_last_seven_days() const {
	fixed_point_t sum = 0;

	for (int i = 0; i <= profit_history_current; i++) {
		sum += daily_profit_history[i];
	}

	return sum / (1 + profit_history_current);
}
