#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
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

	struct PopManager;
	struct IdeologyManager;
	struct GoodDefinitionManager;

	struct NationalFocusManager {
	private:
		IdentifierRegistry<NationalFocusGroup> IDENTIFIER_REGISTRY(national_focus_group);
		IdentifierRegistry<NationalFocus> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(national_focus, national_foci);

	public:
		inline bool add_national_focus_group(std::string_view identifier);

		inline bool add_national_focus(
			std::string_view identifier,
			NationalFocusGroup const& group,
			uint8_t icon,
			bool has_flashpoint,
			fixed_point_t flashpoint_tension,
			bool own_provinces,
			bool outliner_show_as_percent,
			ModifierValue&& modifiers,
			Ideology const* loyalty_ideology,
			fixed_point_t loyalty_value,
			fixed_point_t encourage_railroads,
			fixed_point_map_t<GoodDefinition const*>&& encourage_goods,
			fixed_point_map_t<PopType const*>&& encourage_pop_types,
			ConditionScript&& limit
		);

		bool load_national_foci_file(
			PopManager const& pop_manager, IdeologyManager const& ideology_manager,
			GoodDefinitionManager const& good_definition_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root
		);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
