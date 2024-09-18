#include "ResourceGatheringOperation.hpp"

using namespace OpenVic;

ResourceGatheringOperation::ResourceGatheringOperation(
	ProductionType const& new_production_type,
	fixed_point_t new_size_multiplier,
	fixed_point_t new_revenue_yesterday,
	fixed_point_t new_output_quantity_yesterday,
	fixed_point_t new_unsold_quantity_yesterday,
	ordered_map<Pop*, Pop::pop_size_t>&& new_employees
) : production_type { new_production_type },
	revenue_yesterday { new_revenue_yesterday },
	output_quantity_yesterday { new_output_quantity_yesterday },
	unsold_quantity_yesterday { new_unsold_quantity_yesterday },
	size_multiplier { new_size_multiplier },
	employees { std::move(new_employees) } {}

ResourceGatheringOperation::ResourceGatheringOperation(
	ProductionType const& new_production_type, fixed_point_t new_size_multiplier
) : ResourceGatheringOperation { new_production_type, new_size_multiplier, 0, 0, 0, {} } {}
