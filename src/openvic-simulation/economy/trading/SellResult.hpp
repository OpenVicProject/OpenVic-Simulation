#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct SellResult {
	private:
		GoodDefinition const& PROPERTY(good_definition);
		const fixed_point_t PROPERTY(quantity_sold);
		const fixed_point_t PROPERTY(money_gained);
	public:
		constexpr SellResult(
			GoodDefinition const& new_good_definition,
			const fixed_point_t new_quantity_sold,
			const fixed_point_t new_money_gained
		) : good_definition { new_good_definition },
			quantity_sold { new_quantity_sold },
			money_gained { new_money_gained } {}

		static constexpr SellResult no_sales_result(GoodDefinition const& good_definition) {
			return { good_definition, 0, 0 };
		}
	};
}