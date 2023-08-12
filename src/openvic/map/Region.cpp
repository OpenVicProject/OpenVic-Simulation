#include "Region.hpp"

#include <cassert>

using namespace OpenVic;

return_t ProvinceSet::add_province(Province* province) {
	if (locked) {
		Logger::error("Cannot add province to province set - locked!");
		return FAILURE;
	}
	if (province == nullptr) {
		Logger::error("Cannot add province to province set - null province!");
		return FAILURE;
	}
	if (contains_province(province)) {
		Logger::error("Cannot add province ", province->get_identifier(), " to province set - already in the set!");
		return FAILURE;
	}
	provinces.push_back(province);
	return SUCCESS;
}

void ProvinceSet::lock(bool log) {
	if (locked) {
		Logger::error("Failed to lock province set - already locked!");
	} else {
		locked = true;
		if (log) Logger::info("Locked province set with ", get_province_count(), " provinces");
	}
}

bool ProvinceSet::is_locked() const {
	return locked;
}

void ProvinceSet::reset() {
	provinces.clear();
	locked = false;
}

size_t ProvinceSet::get_province_count() const {
	return provinces.size();
}

bool ProvinceSet::contains_province(Province const* province) const {
	return province && std::find(provinces.begin(), provinces.end(), province) != provinces.end();
}

std::vector<Province*> const& ProvinceSet::get_provinces() const {
	return provinces;
}

Region::Region(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

colour_t Region::get_colour() const {
	if (provinces.empty()) return FULL_COLOUR << 16;
	return provinces.front()->get_colour();
}
