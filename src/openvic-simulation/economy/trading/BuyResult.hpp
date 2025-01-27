#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct BuyResult {
	private:
		const fixed_point_t PROPERTY(quantity_bought);
		const fixed_point_t PROPERTY(money_left);
	public:
		constexpr BuyResult(
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_left
		) : quantity_bought { new_quantity_bought },
			money_left { new_money_left } {}
	};
}