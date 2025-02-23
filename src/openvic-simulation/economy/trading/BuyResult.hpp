#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct BuyResult {
	private:
		GoodDefinition const& PROPERTY(good_definition);
		const fixed_point_t PROPERTY(quantity_bought);
		const fixed_point_t PROPERTY(money_spent);
	public:
		constexpr BuyResult(
			GoodDefinition const& new_good_definition,
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_spent
		) : good_definition { new_good_definition },
			quantity_bought { new_quantity_bought },
			money_spent { new_money_spent } {}

		static constexpr BuyResult no_purchase_result(GoodDefinition const& good_definition) {
			return { good_definition, fixed_point_t::_0(), fixed_point_t::_0() };
		}
	};
}