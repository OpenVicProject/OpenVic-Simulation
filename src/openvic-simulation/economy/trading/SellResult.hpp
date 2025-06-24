#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct SellResult {
	private:
		const fixed_point_t PROPERTY(quantity_sold);
		const fixed_point_t PROPERTY(money_gained);
	public:
		constexpr SellResult(
			const fixed_point_t new_quantity_sold,
			const fixed_point_t new_money_gained
		) : quantity_sold { new_quantity_sold },
			money_gained { new_money_gained } {}

		static constexpr SellResult no_sales_result() {
			return { 0, 0 };
		}
	};
}