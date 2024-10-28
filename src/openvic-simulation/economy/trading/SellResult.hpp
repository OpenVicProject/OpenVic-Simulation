#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct SellResult {
	private:
		fixed_point_t PROPERTY(quantity_sold);
		fixed_point_t PROPERTY(money_gained);
	public:
		SellResult(
			const fixed_point_t new_quantity_sold,
			const fixed_point_t new_money_gained
		);
	};
}