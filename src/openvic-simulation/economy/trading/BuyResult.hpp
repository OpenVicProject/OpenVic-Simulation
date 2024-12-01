#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct BuyResult {
	private:
		fixed_point_t PROPERTY(quantity_bought);
		fixed_point_t PROPERTY(money_left);
	public:
		BuyResult(
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_left
		);
	};
}