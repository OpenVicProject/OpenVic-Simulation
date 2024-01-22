#include "Region.hpp"

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;

bool ProvinceSet::add_province(Province const* province) {
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

bool ProvinceSet::add_provinces(provinces_t const& new_provinces) {
	bool ret = true;
	for (Province const* province : new_provinces) {
		ret &= add_province(province);
	}
	return ret;
}

bool ProvinceSet::remove_province(Province const* province) {
	if (locked) {
		Logger::error("Cannot remove province from province set - locked!");
		return false;
	}
	if (province == nullptr) {
		Logger::error("Cannot remove province from province set - null province!");
		return false;
	}
	const provinces_t::const_iterator it = std::find(provinces.begin(), provinces.end(), province);
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

bool ProvinceSet::contains_province(Province const* province) const {
	return province && std::find(provinces.begin(), provinces.end(), province) != provinces.end();
}

ProvinceSet::provinces_t const& ProvinceSet::get_provinces() const {
	return provinces;
}

Region::Region(std::string_view new_identifier, colour_t new_colour, bool new_meta)
	: HasIdentifierAndColour { new_identifier, new_colour, false }, meta { new_meta } {}

ProvinceSetModifier::ProvinceSetModifier(std::string_view new_identifier, ModifierValue&& new_values)
	: Modifier { new_identifier, std::move(new_values), 0 } {}

bool Region::get_meta() const {
	return meta;
}
