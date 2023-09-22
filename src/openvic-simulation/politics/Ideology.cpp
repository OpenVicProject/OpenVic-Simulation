#include "Ideology.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

IdeologyGroup::IdeologyGroup(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Ideology::Ideology(const std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool new_uncivilised, bool new_can_reduce_militancy, Date new_spawn_date)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false }, group { new_group }, uncivilised { new_uncivilised },
	can_reduce_militancy { new_can_reduce_militancy }, spawn_date { new_spawn_date } {}

IdeologyGroup const& Ideology::get_group() const {
	return group;
}

bool Ideology::is_uncivilised() const {
	return uncivilised;
}

bool Ideology::get_can_reduce_militancy() const {
	return can_reduce_militancy;
}

Date const& Ideology::get_spawn_date() const {
	return spawn_date;
}

IdeologyManager::IdeologyManager() : ideology_groups { "ideology groups" }, ideologies { "ideologies" } {}

bool IdeologyManager::add_ideology_group(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology group identifier - empty!");
		return false;
	}

	return ideology_groups.add_item({ identifier });
}

bool IdeologyManager::add_ideology(const std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised, bool can_reduce_militancy, Date spawn_date) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology identifier - empty!");
		return false;
	}

	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid ideology colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null ideology group for ", identifier);
		return false;
	}

	return ideologies.add_item({ identifier, colour, *group, uncivilised, can_reduce_militancy, spawn_date });
}

/* REQUIREMENTS:
 * POL-9, POL-10, POL-11, POL-12, POL-13, POL-14, POL-15
*/
bool IdeologyManager::load_ideology_file(ast::NodeCPtr root) {
	size_t expected_ideologies = 0;
	bool ret = expect_dictionary_reserve_length(
		ideology_groups,
		[this, &expected_ideologies](std::string_view key, ast::NodeCPtr value) -> bool {
			bool ret = expect_length(add_variable_callback(expected_ideologies))(value);
			ret &= add_ideology_group(key);
			return ret;
		}
	)(root);
	lock_ideology_groups();

	ideologies.reserve(ideologies.size() + expected_ideologies);
	ret &= expect_dictionary(
		[this](std::string_view ideology_group_key, ast::NodeCPtr ideology_group_value) -> bool {
			IdeologyGroup const* ideology_group = get_ideology_group_by_identifier(ideology_group_key);

			return expect_dictionary(
				[this, ideology_group](std::string_view key, ast::NodeCPtr value) -> bool {
					colour_t colour = NULL_COLOUR;
					bool uncivilised = true, can_reduce_militancy = false;
					Date spawn_date;

					bool ret = expect_dictionary_keys(
						"uncivilized", ZERO_OR_ONE, expect_bool(assign_variable_callback(uncivilised)),
						"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
						"date", ZERO_OR_ONE, expect_date(assign_variable_callback(spawn_date)),
						"can_reduce_militancy", ZERO_OR_ONE, expect_bool(assign_variable_callback(can_reduce_militancy)),
						"add_political_reform", ONE_EXACTLY, success_callback,
						"remove_political_reform", ONE_EXACTLY, success_callback,
						"add_social_reform", ONE_EXACTLY, success_callback,
						"remove_social_reform", ONE_EXACTLY, success_callback,
						"add_military_reform", ZERO_OR_ONE, success_callback,
						"add_economic_reform", ZERO_OR_ONE, success_callback
					)(value);
					ret &= add_ideology(key, colour, ideology_group, uncivilised, can_reduce_militancy, spawn_date);
					return ret;
				}
			)(ideology_group_value);
		}
	)(root);
	lock_ideologies();

	return ret;
}