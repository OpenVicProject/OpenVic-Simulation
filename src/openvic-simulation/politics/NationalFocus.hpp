#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct NationalFocusManager;

	struct NationalFocusGroup : HasIdentifier {
	public:
		NationalFocusGroup(std::string_view new_identifier);
	};

	struct Ideology;
	struct GoodDefinition;
	struct PopType;

	struct NationalFocus : Modifier {
		friend struct NationalFocusManager;

	private:
		ConditionScript PROPERTY(limit);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		NationalFocusGroup const& group;
		const uint8_t icon;
		const bool has_flashpoint;
		const fixed_point_t flashpoint_tension;
		const bool own_provinces;
		const bool outliner_show_as_percent;
		Ideology const* const loyalty_ideology;
		const fixed_point_t loyalty_value;
		const fixed_point_t encourage_railroads;
		const fixed_point_map_t<GoodDefinition const*> encourage_goods;
		const fixed_point_map_t<PopType const*> encourage_pop_types;

		NationalFocus(
			std::string_view new_identifier,
			NationalFocusGroup const& new_group,
			uint8_t new_icon,
			bool new_has_flashpoint,
			fixed_point_t new_flashpoint_tension,
			bool new_own_provinces,
			bool new_outliner_show_as_percent,
			ModifierValue&& new_modifiers,
			Ideology const* new_loyalty_ideology,
			fixed_point_t new_loyalty_value,
			fixed_point_t new_encourage_railroads,
			fixed_point_map_t<GoodDefinition const*>&& new_encourage_goods,
			fixed_point_map_t<PopType const*>&& new_encourage_pop_types,
			ConditionScript&& new_limit
		);
		NationalFocus(NationalFocus&&) = default;
	};
}
