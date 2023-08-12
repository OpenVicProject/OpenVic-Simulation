#include "Culture.hpp"

#include <cassert>

using namespace OpenVic;

GraphicalCultureType::GraphicalCultureType(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(const std::string_view new_identifier,
	GraphicalCultureType const& new_unit_graphical_culture_type)
	: HasIdentifier { new_identifier },
	  unit_graphical_culture_type { new_unit_graphical_culture_type } {}

GraphicalCultureType const& CultureGroup::get_unit_graphical_culture_type() const {
	return unit_graphical_culture_type;
}

Culture::Culture(const std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group,
	name_list_t const& new_first_names, name_list_t const& new_last_names)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  group { new_group },
	  first_names { new_first_names },
	  last_names { new_last_names } {
}

CultureGroup const& Culture::get_group() const {
	return group;
}

CultureManager::CultureManager()
	: graphical_culture_types { "graphical culture types" },
	  culture_groups { "culture groups" },
	  cultures { "cultures" } {}

return_t CultureManager::add_graphical_culture_type(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return FAILURE;
	}
	return graphical_culture_types.add_item({ identifier });
}

void CultureManager::lock_graphical_culture_types() {
	graphical_culture_types.lock();
}

GraphicalCultureType const* CultureManager::get_graphical_culture_type_by_identifier(const std::string_view identifier) const {
	return graphical_culture_types.get_item_by_identifier(identifier);
}

return_t CultureManager::add_culture_group(const std::string_view identifier, GraphicalCultureType const* graphical_culture_type) {
	if (!graphical_culture_types.is_locked()) {
		Logger::error("Cannot register culture groups until graphical culture types are locked!");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture group identifier - empty!");
		return FAILURE;
	}
	if (graphical_culture_type == nullptr) {
		Logger::error("Null graphical culture type for ", identifier);
		return FAILURE;
	}
	return culture_groups.add_item({ identifier, *graphical_culture_type });
}

void CultureManager::lock_culture_groups() {
	culture_groups.lock();
}

CultureGroup const* CultureManager::get_culture_group_by_identifier(const std::string_view identifier) const {
	return culture_groups.get_item_by_identifier(identifier);
}

return_t CultureManager::add_culture(const std::string_view identifier, colour_t colour, CultureGroup const* group, Culture::name_list_t const& first_names, Culture::name_list_t const& last_names) {
	if (!culture_groups.is_locked()) {
		Logger::error("Cannot register cultures until culture groups are locked!");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid culture identifier - empty!");
		return FAILURE;
	}
	if (group == nullptr) {
		Logger::error("Null culture group for ", identifier);
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid culture colour for ", identifier, ": ", Culture::colour_to_hex_string(colour));
		return FAILURE;
	}
	// TODO - name list sanatisation?
	return cultures.add_item({ identifier, colour, *group, first_names, last_names });
}

void CultureManager::lock_cultures() {
	cultures.lock();
}

Culture const* CultureManager::get_culture_by_identifier(const std::string_view identifier) const {
	return cultures.get_item_by_identifier(identifier);
}
