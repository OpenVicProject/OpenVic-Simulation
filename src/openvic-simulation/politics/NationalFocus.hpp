#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct NationalFocusManager;

	struct NationalFocusGroup : HasIdentifier {
		friend struct NationalFocusManager;

	private:
		NationalFocusGroup(std::string_view new_identifier);
	};

	struct Ideology;

	struct NationalFocus : Modifier {
		friend struct NationalFocusManager;

	private:
		NationalFocusGroup const& PROPERTY(group);
		uint8_t PROPERTY(icon);
		bool PROPERTY(has_flashpoint);
		bool PROPERTY(own_provinces);
		bool PROPERTY(outliner_show_as_percent);
		Ideology const* PROPERTY(loyalty_ideology);
		fixed_point_t PROPERTY(loyalty_value);
		ConditionScript PROPERTY(limit);

		NationalFocus(
			std::string_view new_identifier,
			NationalFocusGroup const& new_group,
			uint8_t new_icon,
			bool new_has_flashpoint,
			bool new_own_provinces,
			bool new_outliner_show_as_percent,
			ModifierValue&& new_modifiers,
			Ideology const* new_loyalty_ideology,
			fixed_point_t new_loyalty_value,
			ConditionScript&& new_limit
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
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
			bool own_provinces,
			bool outliner_show_as_percent,
			ModifierValue&& modifiers,
			Ideology const* loyalty_ideology,
			fixed_point_t loyalty_value,
			ConditionScript&& limit
		);

		bool load_national_foci_file(
			PopManager const& pop_manager, IdeologyManager const& ideology_manager,
			GoodDefinitionManager const& good_definition_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root
		);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
