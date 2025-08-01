#pragma once

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/types/BuildingLevel.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct BuildingInstance : HasIdentifier { // used in the actual game
		enum class ExpansionState { CannotExpand, CanExpand, Preparing, Expanding };

	private:
		BuildingType const& PROPERTY(building_type);

		building_level_t PROPERTY_RW(level);
		ExpansionState PROPERTY(expansion_state, ExpansionState::CannotExpand);
		Date PROPERTY(start_date);
		Date PROPERTY(end_date);
		fixed_point_t PROPERTY(expansion_progress);

		bool _can_expand() const;

	public:
		BuildingInstance(BuildingType const& new_building_type, building_level_t new_level = 0);
		BuildingInstance(BuildingInstance&&) = default;

		bool expand();
		void update_gamestate(Date today);
		void tick(Date today);
	};
}
