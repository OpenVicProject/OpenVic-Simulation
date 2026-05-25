#include "NationalFocus.hpp"

#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;

NationalFocusGroup::NationalFocusGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

NationalFocus::NationalFocus(
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
) : Modifier { new_identifier, std::move(new_modifiers), modifier_type_t::NATIONAL_FOCUS },
	group { new_group },
	icon { new_icon },
	has_flashpoint { new_has_flashpoint },
	flashpoint_tension { new_flashpoint_tension },
	own_provinces { new_own_provinces },
	outliner_show_as_percent { new_outliner_show_as_percent },
	loyalty_ideology { new_loyalty_ideology },
	loyalty_value { new_loyalty_value },
	encourage_railroads { new_encourage_railroads },
	encourage_goods { std::move(new_encourage_goods) },
	encourage_pop_types { std::move(new_encourage_pop_types) },
	limit { std::move(new_limit) } {}

bool NationalFocus::parse_scripts(DefinitionManager const& definition_manager) {
	spdlog::scope scope { fmt::format("national focus {}", get_identifier()) };
	return limit.parse_script(true, definition_manager);
}
