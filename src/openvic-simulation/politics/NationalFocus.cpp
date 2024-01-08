#include "NationalFocus.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

NationalFocusGroup::NationalFocusGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

NationalFocus::NationalFocus(
	std::string_view new_identifier,
	uint8_t new_icon,
	NationalFocusGroup const& new_group,
	ModifierValue&& new_modifiers,
	pop_promotion_map_t&& new_encouraged_promotion,
	production_map_t&& new_encouraged_production,
	Ideology const* new_loyalty_ideology,
	fixed_point_t new_loyalty_value,
	ConditionScript&& new_limit
) : HasIdentifier { new_identifier },
	icon { new_icon },
	group { new_group },
	modifiers { std::move(new_modifiers) },
	encouraged_promotion { std::move(new_encouraged_promotion) },
	encouraged_production { std::move(new_encouraged_production) },
	loyalty_ideology { new_loyalty_ideology },
	loyalty_value { new_loyalty_value },
	limit { std::move(new_limit) } {}

bool NationalFocus::parse_scripts(GameManager const& game_manager) {
	return limit.parse_script(true, game_manager);
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
	uint8_t icon,
	NationalFocusGroup const& group,
	ModifierValue&& modifiers,
	NationalFocus::pop_promotion_map_t&& encouraged_promotion,
	NationalFocus::production_map_t&& encouraged_production,
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
		identifier, icon, group, std::move(modifiers), std::move(encouraged_promotion), std::move(encouraged_production),
		loyalty_ideology, loyalty_value, std::move(limit)
	});
}

bool NationalFocusManager::load_national_foci_file(
	PopManager const& pop_manager, IdeologyManager const& ideology_manager, GoodManager const& good_manager,
	ModifierManager const& modifier_manager, ast::NodeCPtr root
) {
	size_t expected_national_foci = 0;
	bool ret = expect_dictionary_reserve_length(
		national_focus_groups,
		[this, &expected_national_foci](std::string_view identifier, ast::NodeCPtr node) -> bool {
			return expect_length(add_variable_callback(expected_national_foci))(node) & add_national_focus_group(identifier);
		}
	)(root);
	lock_national_focus_groups();

	national_foci.reserve(expected_national_foci);

	ret &= expect_national_focus_group_dictionary([this, &pop_manager, &ideology_manager, &good_manager, &modifier_manager](
		NationalFocusGroup const& group, ast::NodeCPtr group_node
	) -> bool {
		return expect_dictionary([this, &group, &pop_manager, &ideology_manager, &good_manager, &modifier_manager](
			std::string_view identifier, ast::NodeCPtr node
		) -> bool {
			uint8_t icon = 0;
			ModifierValue modifiers;
			NationalFocus::pop_promotion_map_t promotions;
			NationalFocus::production_map_t production;
			Ideology const* loyalty_ideology = nullptr;
			fixed_point_t loyalty_value = 0;
			ConditionScript limit { scope_t::PROVINCE | scope_t::COUNTRY, scope_t::PROVINCE | scope_t::COUNTRY, scope_t::NO_SCOPE };

			bool ret = modifier_manager.expect_modifier_value_and_keys_and_default(
				move_variable_callback(modifiers),
				[&promotions, &pop_manager, &production, &good_manager, &modifiers, &modifier_manager](
					std::string_view key, ast::NodeCPtr value
				) -> bool {
					PopType const* pop_type = pop_manager.get_pop_type_by_identifier(key);
					if (pop_type != nullptr) {
						return expect_fixed_point(map_callback(promotions, pop_type))(value);
					}
					Good const* good = good_manager.get_good_by_identifier(key);
					if (good != nullptr) {
						return expect_fixed_point(map_callback(production, good))(value);
					}
					return key_value_invalid_callback(key, value);
				},
				"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
				"ideology", ZERO_OR_ONE,
					ideology_manager.expect_ideology_identifier(assign_variable_callback_pointer(loyalty_ideology)),
				"loyalty_value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(loyalty_value)),
				"limit", ZERO_OR_ONE, limit.expect_script(),
				"has_flashpoint", ZERO_OR_ONE, success_callback, // special case, include in limit
				"own_provinces", ZERO_OR_ONE, success_callback, // special case, include in limit
				"outliner_show_as_percent", ZERO_OR_ONE, success_callback // special case
			)(node);

			ret &= add_national_focus(
				identifier, icon, group, std::move(modifiers), std::move(promotions), std::move(production),
				loyalty_ideology, loyalty_value, std::move(limit)
			);

			return ret;
		})(group_node);
	})(root);
	lock_national_foci();

	return ret;
}

bool NationalFocusManager::parse_scripts(GameManager const& game_manager) {
	bool ret = true;
	for (NationalFocus& national_focus : national_foci.get_items()) {
		ret &= national_focus.parse_scripts(game_manager);
	}
	return ret;
}
