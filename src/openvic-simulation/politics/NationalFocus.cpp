#include "NationalFocus.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

NationalFocusGroup::NationalFocusGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

NationalFocus::NationalFocus(
	std::string_view new_identifier,
	uint8_t new_icon,
	NationalFocusGroup const& new_group,
	ModifierValue&& new_modifiers,
	pop_promotion_map_t&& new_encouraged_promotion,
	party_loyalty_map_t&& new_encouraged_loyalty,
	production_map_t&& new_encouraged_production
) : HasIdentifier { new_identifier },
	icon { new_icon },
	group { new_group },
	modifiers { std::move(new_modifiers) },
	encouraged_promotion { std::move(new_encouraged_promotion) },
	encouraged_loyalty { std::move(new_encouraged_loyalty) },
	encouraged_production { std::move(new_encouraged_production) } {}

NationalFocusManager::NationalFocusManager() : national_focus_groups { "national focus groups" }, national_foci { "national foci" } {}

inline bool NationalFocusManager::add_national_focus_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("No identifier for national focus group!");
		return false;
	}
	return national_focus_groups.add_item({ identifier });
}

inline bool NationalFocusManager::add_national_focus(
	std::string_view identifier,
	uint8_t icon,
	NationalFocusGroup const& group,
	ModifierValue&& modifiers,
	NationalFocus::pop_promotion_map_t&& encouraged_promotion,
	NationalFocus::party_loyalty_map_t&& encouraged_loyalty,
	NationalFocus::production_map_t&& encouraged_production
) {
	if (identifier.empty()) {
		Logger::error("No identifier for national focus!");
		return false;
	}
	if (icon < 1) {
		Logger::error("Invalid icon ", icon, " for national focus ", identifier);
		return false;
	}
	return national_foci.add_item({ identifier, icon, group, std::move(modifiers), std::move(encouraged_promotion), std::move(encouraged_loyalty), std::move(encouraged_production) });
}

bool NationalFocusManager::load_national_foci_file(PopManager const& pop_manager, IdeologyManager const& ideology_manager, GoodManager const& good_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(national_focus_groups, [this](std::string_view identifier, ast::NodeCPtr node) -> bool {
		return add_national_focus_group(identifier);
	})(root);
	lock_national_focus_groups();

	ret &= expect_national_focus_group_dictionary([this, &pop_manager, &ideology_manager, &good_manager, &modifier_manager](NationalFocusGroup const& group, ast::NodeCPtr node) -> bool {
		bool ret = expect_dictionary([this, &group, &pop_manager, &ideology_manager, &good_manager, &modifier_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {
			uint8_t icon;
			ModifierValue modifiers;
			NationalFocus::pop_promotion_map_t promotions;
			NationalFocus::party_loyalty_map_t loyalties;
			NationalFocus::production_map_t production;

			Ideology const* last_specified_ideology = nullptr; // weird, I know

			bool ret = modifier_manager.expect_modifier_value_and_keys_and_default(
				move_variable_callback(modifiers),
				[&promotions, &pop_manager, &production, &good_manager, &modifiers, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
					PopType const* pop_type = pop_manager.get_pop_type_by_identifier(key);
					if (pop_type != nullptr) {
						fixed_point_t boost;
						bool ret = expect_fixed_point(assign_variable_callback(boost))(value);
						promotions[pop_type] = boost;
						return ret;
					}
					Good const* good = good_manager.get_good_by_identifier(key);
					if (good != nullptr) {
						fixed_point_t boost;
						bool ret = expect_fixed_point(assign_variable_callback(boost))(value);
						production[good] = boost;
						return ret;
					}
					return key_value_invalid_callback(key, value);
				},
				"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
				"ideology", ZERO_OR_MORE, ideology_manager.expect_ideology_identifier(assign_variable_callback_pointer(last_specified_ideology)),
				"loyalty_value", ZERO_OR_MORE, [&identifier, &last_specified_ideology, &loyalties](ast::NodeCPtr value) -> bool {
					if (last_specified_ideology == nullptr) {
						Logger::error("In national focus ", identifier, ": No ideology selected for loyalty_value!");
						return false;
					}
					fixed_point_t boost;
					bool ret = expect_fixed_point(assign_variable_callback(boost))(value);
					loyalties[last_specified_ideology] += boost;
					return ret;
				},
				"limit", ZERO_OR_ONE, success_callback, // TODO: implement conditions
				"has_flashpoint", ZERO_OR_ONE, success_callback, // special case, include in limit
				"own_provinces", ZERO_OR_ONE, success_callback, // special case, include in limit
				"outliner_show_as_percent", ZERO_OR_ONE, success_callback // special case
			)(node);

			add_national_focus(identifier, icon, group, std::move(modifiers), std::move(promotions), std::move(loyalties), std::move(production));

			return ret;
		})(node);
		return ret;
	})(root);
	lock_national_foci();

	return ret;
}
