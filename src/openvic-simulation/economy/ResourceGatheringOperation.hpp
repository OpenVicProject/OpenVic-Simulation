#pragma once

#include "openvic-simulation/economy/ProductionType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	class ResourceGatheringOperation final {
	private:
		ProductionType const& PROPERTY(production_type);
		fixed_point_t PROPERTY(revenue_yesterday);
		fixed_point_t PROPERTY(output_quantity_yesterday);
		fixed_point_t PROPERTY(unsold_quantity_yesterday);
		fixed_point_t PROPERTY(size_multiplier);
		ordered_map<Pop*, Pop::pop_size_t> PROPERTY(employees);

	public:
		ResourceGatheringOperation(
			ProductionType const& new_production_type,
			fixed_point_t new_size_multiplier,
			fixed_point_t new_revenue_yesterday,
			fixed_point_t new_output_quantity_yesterday,
			fixed_point_t new_unsold_quantity_yesterday,
			ordered_map<Pop*, Pop::pop_size_t>&& new_employees
		);
		ResourceGatheringOperation(ProductionType const& new_production_type, fixed_point_t new_size_multiplier);
	};
}
