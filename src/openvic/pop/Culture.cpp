#include "Culture.hpp"

#include "openvic/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GraphicalCultureType::GraphicalCultureType(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(const std::string_view new_identifier, const std::string_view new_leader,
	GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas)
	: HasIdentifier { new_identifier }, leader { new_leader },
	  unit_graphical_culture_type { new_unit_graphical_culture_type },
	  is_overseas { new_is_overseas } {}


std::string const& CultureGroup::get_leader() const {
	return leader;
}

GraphicalCultureType const& CultureGroup::get_unit_graphical_culture_type() const {
	return unit_graphical_culture_type;
}

bool CultureGroup::get_is_overseas() const {
	return is_overseas;
}

Culture::Culture(const std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group,
	std::vector<std::string> const& new_first_names, std::vector<std::string> const& new_last_names)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  group { new_group },
	  first_names { new_first_names },
	  last_names { new_last_names } {}

CultureGroup const& Culture::get_group() const {
	return group;
}

std::vector<std::string> const& Culture::get_first_names() const {
	return first_names;
}

std::vector<std::string> const& Culture::get_last_names() const {
	return last_names;
}

CultureManager::CultureManager()
	: graphical_culture_types { "graphical culture types" },
	  culture_groups { "culture groups" },
	  cultures { "cultures" } {}

bool CultureManager::add_graphical_culture_type(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	return graphical_culture_types.add_item({ identifier });
}

void CultureManager::lock_graphical_culture_types() {
	graphical_culture_types.lock();
}

GraphicalCultureType const* CultureManager::get_graphical_culture_type_by_identifier(const std::string_view identifier) const {
	return graphical_culture_types.get_item_by_identifier(identifier);
}

size_t CultureManager::get_graphical_culture_type_count() const {
	return graphical_culture_types.size(); 
}

std::vector<GraphicalCultureType> const& CultureManager::get_graphical_culture_types() const {
	return graphical_culture_types.get_items();
}

bool CultureManager::add_culture_group(const std::string_view identifier, const std::string_view leader, GraphicalCultureType const* graphical_culture_type, bool is_overseas) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot register culture groups until graphical culture types are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return false;
	}
	if (leader.empty()) {
		Logger::error("Invalid culture group leader - empty!");
		return false;
	}
	if (graphical_culture_type == nullptr) {
		Logger::error("Null graphical culture type for ", identifier);
		return false;
	}
	return culture_groups.add_item({ identifier, leader, *graphical_culture_type, is_overseas });
}

void CultureManager::lock_culture_groups() {
	culture_groups.lock();
}

CultureGroup const* CultureManager::get_culture_group_by_identifier(const std::string_view identifier) const {
	return culture_groups.get_item_by_identifier(identifier);
}

size_t CultureManager::get_culture_group_count() const {
	return culture_groups.size(); 
}

std::vector<CultureGroup> const& CultureManager::get_culture_groups() const {
	return culture_groups.get_items();
}

bool CultureManager::add_culture(const std::string_view identifier, colour_t colour, CultureGroup const* group, std::vector<std::string> const& first_names, std::vector<std::string> const& last_names) {
	if (!culture_groups.is_locked()) {
		Logger::error("Cannot register cultures until culture groups are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture identifier - empty!");
		return false;
	}
	if (group == nullptr) {
		Logger::error("Null culture group for ", identifier);
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid culture colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	return cultures.add_item({ identifier, colour, *group, first_names, last_names });
}

void CultureManager::lock_cultures() {
	cultures.lock();
}

Culture const* CultureManager::get_culture_by_identifier(const std::string_view identifier) const {
	return cultures.get_item_by_identifier(identifier);
}

size_t CultureManager::get_culture_count() const {
	return cultures.size(); 
}

std::vector<Culture> const& CultureManager::get_cultures() const {
	return cultures.get_items();
}

bool CultureManager::load_graphical_culture_type_file(ast::NodeCPtr root) {
	const bool ret = expect_list_reserve_length(
		graphical_culture_types,
		expect_identifier(
			std::bind(&CultureManager::add_graphical_culture_type, this, std::placeholders::_1)
		)
	)(root);
	lock_graphical_culture_types();
	return ret;
}

bool CultureManager::load_culture_file(ast::NodeCPtr root) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot load culture groups until graphical culture types are locked!");
		return false;
	}

	static const std::string default_unit_graphical_culture_type_identifier = "Generic";
	GraphicalCultureType const* const default_unit_graphical_culture_type = get_graphical_culture_type_by_identifier(default_unit_graphical_culture_type_identifier);
	if (default_unit_graphical_culture_type == nullptr) {
		Logger::error("Failed to find default unit graphical culture type: ", default_unit_graphical_culture_type_identifier);
	}

	size_t total_expected_cultures = 0;
	bool ret = expect_dictionary_reserve_length(
		culture_groups,
		[this, default_unit_graphical_culture_type, &total_expected_cultures](std::string_view key, ast::NodeCPtr value) -> bool {
			std::string_view leader;
			GraphicalCultureType const* unit_graphical_culture_type = default_unit_graphical_culture_type;
			bool is_overseas = true;

			bool ret = expect_dictionary_keys_and_length(
				[&total_expected_cultures](size_t size) -> size_t {
					total_expected_cultures += size;
					return size;
				},
				ALLOW_OTHER_KEYS,
				"leader", ONE_EXACTLY, expect_identifier(assign_variable_callback(leader)),
				"unit", ZERO_OR_ONE,
					expect_identifier(
						[this, &unit_graphical_culture_type](std::string_view identifier) -> bool {
							unit_graphical_culture_type = get_graphical_culture_type_by_identifier(identifier);
							if (unit_graphical_culture_type != nullptr) return true;
							Logger::error("Invalid unit graphical culture type: ", identifier);
							return false;
						}
					),
				"union", ZERO_OR_ONE, success_callback,
				"is_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_overseas))
			)(value);
			ret &= add_culture_group(key, leader, unit_graphical_culture_type, is_overseas);
			return ret;
		}
	)(root);
	lock_culture_groups();
	cultures.reserve(cultures.size() + total_expected_cultures);

	ret &= expect_dictionary(
		[this](std::string_view culture_group_key, ast::NodeCPtr culture_group_value) -> bool {

			CultureGroup const* culture_group = get_culture_group_by_identifier(culture_group_key);

			return expect_dictionary(
				[this, culture_group](std::string_view key, ast::NodeCPtr value) -> bool {
					if (key == "leader" || key == "unit" || key == "union" || key == "is_overseas") return true;

					colour_t colour = NULL_COLOUR;
					std::vector<std::string> first_names, last_names;

					bool ret = expect_dictionary_keys(
						"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
						"first_names", ONE_EXACTLY, name_list_callback(first_names),
						"last_names", ONE_EXACTLY, name_list_callback(last_names),
						"radicalism", ZERO_OR_ONE, success_callback,
						"primary", ZERO_OR_ONE, success_callback
					)(value);
					ret &= add_culture(key, colour, culture_group, first_names, last_names);
					return ret;
				}
			)(culture_group_value);
		}
	)(root);
	lock_cultures();
	return ret;
}
