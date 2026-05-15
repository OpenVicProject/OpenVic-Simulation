#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct BuyResult {
	public:
		const good_index_t good_index;
		const fixed_point_t quantity_bought;
		const fixed_point_t money_spent_total;
		const fixed_point_t money_spent_on_imports;

		constexpr BuyResult(
			const good_index_t new_good_index,
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_spent_total,
			const fixed_point_t new_money_spent_on_imports
		) : good_index { new_good_index },
			quantity_bought { new_quantity_bought },
			money_spent_total { new_money_spent_total },
			money_spent_on_imports { new_money_spent_on_imports } {}

		static constexpr BuyResult no_purchase_result(const good_index_t good_index) {
			return { good_index, 0, 0, 0 };
		}
	};
}