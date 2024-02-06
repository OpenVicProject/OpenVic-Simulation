#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	class ArtisanalProducer final {
	private:
		ProductionType const& PROPERTY(production_type);
		Good::good_map_t PROPERTY(stockpile);
		fixed_point_t PROPERTY(current_production);
		Good::good_map_t PROPERTY(current_needs);

	public:
		ArtisanalProducer(
			ProductionType const& new_production_type, Good::good_map_t&& new_stockpile,
			const fixed_point_t new_current_production, Good::good_map_t&& new_current_needs
		);
	};
}
