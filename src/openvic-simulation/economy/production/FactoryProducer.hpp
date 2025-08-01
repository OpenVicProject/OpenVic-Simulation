#pragma once

#include <cstdint>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodDefinition;
	struct ProductionType;
	struct Pop;

	struct FactoryProducer {
	private:
		static constexpr uint8_t DAYS_OF_HISTORY = 7;
		using daily_profit_history_t = std::array<fixed_point_t, DAYS_OF_HISTORY>;

		uint8_t PROPERTY(profit_history_current);
		daily_profit_history_t PROPERTY(daily_profit_history);
		ProductionType const& PROPERTY(production_type);
		fixed_point_t PROPERTY(revenue_yesterday);
		fixed_point_t PROPERTY(output_quantity_yesterday);
		fixed_point_t PROPERTY(unsold_quantity_yesterday);
		fixed_point_t PROPERTY(size_multiplier);
		ordered_map<Pop*, pop_size_t> PROPERTY(employees);
		fixed_point_map_t<GoodDefinition const*> PROPERTY(stockpile);
		fixed_point_t PROPERTY(budget);
		fixed_point_t PROPERTY(balance_yesterday);
		fixed_point_t PROPERTY(received_investments_yesterday);
		fixed_point_t PROPERTY(market_spendings_yesterday);
		fixed_point_t PROPERTY(paychecks_yesterday);
		uint32_t PROPERTY(unprofitable_days);
		uint32_t PROPERTY(subsidised_days);
		uint32_t PROPERTY(days_without_input);
		uint8_t PROPERTY_RW(hiring_priority);

	public:
		FactoryProducer(
			ProductionType const& new_production_type, fixed_point_t new_size_multiplier, fixed_point_t new_revenue_yesterday,
			fixed_point_t new_output_quantity_yesterday, fixed_point_t new_unsold_quantity_yesterday,
			ordered_map<Pop*, pop_size_t>&& new_employees, fixed_point_map_t<GoodDefinition const*>&& new_stockpile,
			fixed_point_t new_budget, fixed_point_t new_balance_yesterday, fixed_point_t new_received_investments_yesterday,
			fixed_point_t new_market_spendings_yesterday, fixed_point_t new_paychecks_yesterday, uint32_t new_unprofitable_days,
			uint32_t new_subsidised_days, uint32_t new_days_without_input, uint8_t new_hiring_priority,
			uint8_t new_profit_history_current, daily_profit_history_t&& new_daily_profit_history
		);
		FactoryProducer(ProductionType const& new_production_type, fixed_point_t new_size_multiplier);

		fixed_point_t get_profitability_yesterday() const;
		fixed_point_t get_average_profitability_last_seven_days() const;
	};
}
