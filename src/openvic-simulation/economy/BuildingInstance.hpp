#pragma once

#include "openvic-simulation/economy/BuildingLevel.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct BuildingType;
	struct CountryInstance;
	struct ModifierEffectCache;
	struct ProvinceInstance;

	struct BuildingInstance : HasIdentifier { // used in the actual game
		enum class ExpansionState { CannotExpand, CanExpand, Preparing, Expanding };

	private:
		building_level_t PROPERTY(level);
		ExpansionState PROPERTY(expansion_state, ExpansionState::CannotExpand);
		Date PROPERTY(start_date);
		Date PROPERTY(end_date);
		fixed_point_t PROPERTY(expansion_progress);

		bool _can_expand() const;

	public:
		BuildingType const& building_type;

		BuildingInstance(BuildingType const& new_building_type, building_level_t new_level = building_level_t { 0 });
		BuildingInstance(BuildingInstance&&) = default;

		bool expand(
			ModifierEffectCache const& modifier_effect_cache,
			CountryInstance& actor,
			ProvinceInstance const& location
		);
		void update_gamestate(Date today);
		void tick(Date today);
		void set_level(const building_level_t new_level);
	};
}
