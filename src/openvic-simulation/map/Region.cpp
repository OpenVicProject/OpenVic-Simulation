#include "Region.hpp"

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;

bool ProvinceSet::add_province(ProvinceDefinition const* province) {
	if (locked) {
		Logger::error("Cannot add province to province set - locked!");
		return false;
	}

	if (province == nullptr) {
		Logger::error("Cannot add province to province set - null province!");
		return false;
	}

	if (contains_province(province)) {
		Logger::warning("Cannot add province ", province->get_identifier(), " to province set - already in the set!");
		return false;
	}

	provinces.push_back(province);

	return true;
}

bool ProvinceSet::remove_province(ProvinceDefinition const* province) {
	if (locked) {
		Logger::error("Cannot remove province from province set - locked!");
		return false;
	}

	if (province == nullptr) {
		Logger::error("Cannot remove province from province set - null province!");
		return false;
	}

	const decltype(provinces)::const_iterator it = std::find(provinces.begin(), provinces.end(), province);
	if (it == provinces.end()) {
		Logger::warning("Cannot remove province ", province->get_identifier(), " from province set - already not in the set!");
		return false;
	}

	provinces.erase(it);

	return true;
}

void ProvinceSet::lock(bool log) {
	if (locked) {
		Logger::error("Failed to lock province set - already locked!");
	} else {
		locked = true;
		if (log) {
			Logger::info("Locked province set with ", size(), " provinces");
		}
	}
}

bool ProvinceSet::is_locked() const {
	return locked;
}

void ProvinceSet::reset() {
	provinces.clear();
	locked = false;
}

bool ProvinceSet::empty() const {
	return provinces.empty();
}

size_t ProvinceSet::size() const {
	return provinces.size();
}

size_t ProvinceSet::capacity() const {
	return provinces.capacity();
}

void ProvinceSet::reserve(size_t size) {
	if (locked) {
		Logger::error("Failed to reserve space for ", size, " items in province set - already locked!");
	} else {
		provinces.reserve(size);
	}
}

void ProvinceSet::reserve_more(size_t size) {
	OpenVic::reserve_more(*this, size);
}

bool ProvinceSet::contains_province(ProvinceDefinition const* province) const {
	return province != nullptr && std::find(provinces.begin(), provinces.end(), province) != provinces.end();
}

ProvinceSetModifier::ProvinceSetModifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type)
	: Modifier { new_identifier, std::move(new_values), new_type } {}

Region::Region(std::string_view new_identifier, colour_t new_colour, bool new_is_meta)
	: HasIdentifierAndColour { new_identifier, new_colour, false }, is_meta { new_is_meta } {}
