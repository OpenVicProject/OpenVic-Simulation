#include "Religion.hpp"

#include <cassert>

using namespace OpenVic;

ReligionGroup::ReligionGroup(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Religion::Religion(const std::string_view new_identifier, colour_t new_colour,
	ReligionGroup const& new_group, icon_t new_icon, bool new_pagan)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  group { new_group },
	  icon { new_icon },
	  pagan { new_pagan } {
	assert(icon > 0);
}

ReligionGroup const& Religion::get_group() const {
	return group;
}

Religion::icon_t Religion::get_icon() const {
	return icon;
}

bool Religion::get_pagan() const {
	return pagan;
}

ReligionManager::ReligionManager()
	: religion_groups { "religion groups" },
	  religions { "religions" } {}

return_t ReligionManager::add_religion_group(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid religion group identifier - empty!");
		return FAILURE;
	}
	return religion_groups.add_item({ identifier });
}

void ReligionManager::lock_religion_groups() {
	religion_groups.lock();
}

ReligionGroup const* ReligionManager::get_religion_group_by_identifier(const std::string_view identifier) const {
	return religion_groups.get_item_by_identifier(identifier);
}

return_t ReligionManager::add_religion(const std::string_view identifier, colour_t colour, ReligionGroup const* group, Religion::icon_t icon, bool pagan) {
	if (!religion_groups.is_locked()) {
		Logger::error("Cannot register religions until religion groups are locked!");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid religion identifier - empty!");
		return FAILURE;
	}
	if (group == nullptr) {
		Logger::error("Null religion group for ", identifier);
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid religion colour for ", identifier, ": ", Religion::colour_to_hex_string(colour));
		return FAILURE;
	}
	if (icon <= 0) {
		Logger::error("Invalid religion icon for ", identifier, ": ", icon);
		return FAILURE;
	}
	return religions.add_item({ identifier, colour, *group, icon, pagan });
}

void ReligionManager::lock_religions() {
	religions.lock();
}

Religion const* ReligionManager::get_religion_by_identifier(const std::string_view identifier) const {
	return religions.get_item_by_identifier(identifier);
}

return_t ReligionManager::load_religion_file(ast::NodeCPtr root) {

	// TODO - religion loading

	return_t ret = add_religion_group("test_religion_group");
	lock_religion_groups();
	if (add_religion("test_religion", 0xFF0000, get_religion_group_by_identifier("test_religion_group"), 1, false) != SUCCESS)
		ret = FAILURE;
	lock_religions();
	return ret;
}
