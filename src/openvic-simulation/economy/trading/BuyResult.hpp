#pragma once

#include "openvic-simulation/core/object/FixedPoint.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct BuyResult {
	private:
		GoodDefinition const& PROPERTY(good_definition);
		const fixed_point_t PROPERTY(quantity_bought);
		const fixed_point_t PROPERTY(money_spent_total);
		const fixed_point_t PROPERTY(money_spent_on_imports);
	public:
		constexpr BuyResult(
			GoodDefinition const& new_good_definition,
			const fixed_point_t new_quantity_bought,
			const fixed_point_t new_money_spent_total,
			const fixed_point_t new_money_spent_on_imports
		) : good_definition { new_good_definition },
			quantity_bought { new_quantity_bought },
			money_spent_total { new_money_spent_total },
			money_spent_on_imports { new_money_spent_on_imports } {}

		static constexpr BuyResult no_purchase_result(GoodDefinition const& good_definition) {
			return { good_definition, 0, 0, 0 };
		}
	};
}