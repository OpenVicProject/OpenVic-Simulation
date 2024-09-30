#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ArtisanalProducer {
	private:
		ProductionType const& PROPERTY(production_type);
		GoodDefinition::good_definition_map_t PROPERTY(stockpile);
		fixed_point_t PROPERTY(current_production);
		GoodDefinition::good_definition_map_t PROPERTY(current_needs);

	public:
		ArtisanalProducer(
			ProductionType const& new_production_type, GoodDefinition::good_definition_map_t&& new_stockpile,
			fixed_point_t new_current_production, GoodDefinition::good_definition_map_t&& new_current_needs
		);
	};
}
