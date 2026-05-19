#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct SellResult {
	public:
		const good_index_t good_index;
		const fixed_point_t quantity_sold;
		const fixed_point_t money_gained;

		constexpr SellResult(
			const good_index_t new_good_index,
			const fixed_point_t new_quantity_sold,
			const fixed_point_t new_money_gained
		) : good_index { new_good_index },
			quantity_sold { new_quantity_sold },
			money_gained { new_money_gained } {}

		static constexpr SellResult no_sales_result(const good_index_t good_index) {
			return { good_index, 0, 0 };
		}
	};
}