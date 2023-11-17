#pragma once

#include "openvic-simulation/economy/BuildingType.hpp"

namespace OpenVic {

	struct BuildingInstance : HasIdentifier { // used in the actual game
		using level_t = BuildingType::level_t;

		enum class ExpansionState { CannotExpand, CanExpand, Preparing, Expanding };

	private:
		BuildingType const& PROPERTY(building_type);

		level_t PROPERTY(level);
		ExpansionState PROPERTY(expansion_state);
		Date PROPERTY(start_date)
		Date PROPERTY(end_date);
		float PROPERTY(expansion_progress);

		bool _can_expand() const;

	public:
		BuildingInstance(BuildingType const& new_building_type, level_t new_level = 0);
		BuildingInstance(BuildingInstance&&) = default;

		void set_level(level_t new_level);

		bool expand();
		void update_state(Date today);
		void tick(Date today);
	};
}
