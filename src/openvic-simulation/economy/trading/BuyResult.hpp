#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct BuyResult {
	private:
		const fixed_point_t PROPERTY(quantity_bought);
		const fixed_point_t PROPERTY(money_spent);
	public:
		constexpr BuyResult(
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_spent
		) : quantity_bought { new_quantity_bought },
			money_spent { new_money_spent } {}

		static constexpr BuyResult no_purchase_result() {
			return { fixed_point_t::_0(), fixed_point_t::_0() };
		}
	};
}