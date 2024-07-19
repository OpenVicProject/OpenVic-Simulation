#include "NationalFocus.hpp"

#include "openvic-simulation/politics/Ideology.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

NationalFocusGroup::NationalFocusGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

NationalFocus::NationalFocus(
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
) : Modifier { new_identifier, std::move(new_modifiers) },
	group { new_group },
	icon { new_icon },
	has_flashpoint { new_has_flashpoint },
	own_provinces { new_own_provinces },
	outliner_show_as_percent { new_outliner_show_as_percent },
	loyalty_ideology { new_loyalty_ideology },
	loyalty_value { new_loyalty_value },
	limit { std::move(new_limit) } {}

bool NationalFocus::parse_scripts(DefinitionManager const& definition_manager) {
	return limit.parse_script(true, definition_manager);
}

inline bool NationalFocusManager::add_national_focus_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("No identifier for national focus group!");
		return false;
	}
	return national_focus_groups.add_item({ identifier });
}

inline bool NationalFocusManager::add_national_focus(
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
) {
	if (identifier.empty()) {
		Logger::error("No identifier for national focus!");
		return false;
	}
	if (icon < 1) {
		Logger::error("Invalid icon ", icon, " for national focus ", identifier);
		return false;
	}
	if ((loyalty_ideology == nullptr) != (loyalty_value == 0)) {
		Logger::warning(
			"Party loyalty incorrectly defined for national focus ", identifier, ": ideology = ", loyalty_ideology,
			", value = ", loyalty_value
		);
	}
	return national_foci.add_item({
		identifier, group, icon, has_flashpoint, own_provinces, outliner_show_as_percent, std::move(modifiers),
		loyalty_ideology, loyalty_value, std::move(limit)
	});
}

bool NationalFocusManager::load_national_foci_file(
	PopManager const& pop_manager, IdeologyManager const& ideology_manager,
	GoodDefinitionManager const& good_definition_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root
) {
	size_t expected_national_foci = 0;
	bool ret = expect_dictionary_reserve_length(
		national_focus_groups,
		[this, &expected_national_foci](std::string_view identifier, ast::NodeCPtr node) -> bool {
			return expect_length(add_variable_callback(expected_national_foci))(node) & add_national_focus_group(identifier);
		}
	)(root);
	lock_national_focus_groups();

	reserve_more_national_foci(expected_national_foci);

	ret &= expect_national_focus_group_dictionary(
		[this, &pop_manager, &ideology_manager, &good_definition_manager, &modifier_manager](
			NationalFocusGroup const& group, ast::NodeCPtr group_node
		) -> bool {
			return expect_dictionary(
				[this, &group, &pop_manager, &ideology_manager, &good_definition_manager, &modifier_manager](
					std::string_view identifier, ast::NodeCPtr node
				) -> bool {
					uint8_t icon = 0;
					bool has_flashpoint = false, own_provinces = true, outliner_show_as_percent = false;
					ModifierValue modifiers;
					Ideology const* loyalty_ideology = nullptr;
					fixed_point_t loyalty_value = 0;
					ConditionScript limit {
						scope_t::PROVINCE | scope_t::COUNTRY, scope_t::PROVINCE | scope_t::COUNTRY, scope_t::NO_SCOPE
					};

					bool ret = modifier_manager.expect_modifier_value_and_keys(
						move_variable_callback(modifiers),
						"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
						"ideology", ZERO_OR_ONE,
							ideology_manager.expect_ideology_identifier(assign_variable_callback_pointer(loyalty_ideology)),
						"loyalty_value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(loyalty_value)),
						"limit", ZERO_OR_ONE, limit.expect_script(),
						"has_flashpoint", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_flashpoint)),
						"own_provinces", ZERO_OR_ONE, expect_bool(assign_variable_callback(own_provinces)),
						"outliner_show_as_percent", ZERO_OR_ONE,
							expect_bool(assign_variable_callback(outliner_show_as_percent))
					)(node);

					ret &= add_national_focus(
						identifier, group, icon, has_flashpoint, own_provinces, outliner_show_as_percent, std::move(modifiers),
						loyalty_ideology, loyalty_value, std::move(limit)
					);

					return ret;
				}
			)(group_node);
		}
	)(root);
	lock_national_foci();

	return ret;
}

bool NationalFocusManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (NationalFocus& national_focus : national_foci.get_items()) {
		ret &= national_focus.parse_scripts(definition_manager);
	}
	return ret;
}
