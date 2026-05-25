#pragma once

#include "openvic-simulation/politics/NationalFocus.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
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
			GoodDefinitionManager const& good_definition_manager, ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root
		);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
