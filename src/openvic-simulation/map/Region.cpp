#include "Region.hpp"

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/core/object/Colour.hpp"

using namespace OpenVic;

bool ProvinceSet::add_province(ProvinceDefinition const* province) {
	if (locked) {
		spdlog::error_s("Cannot add province to province set - locked!");
		return false;
	}

	if (province == nullptr) {
		spdlog::error_s("Cannot add province to province set - null province!");
		return false;
	}

	if (contains_province(province)) {
		spdlog::warn_s("Cannot add province {} to province set - already in the set!", *province);
		return false;
	}

	provinces.push_back(province);

	return true;
}

bool ProvinceSet::remove_province(ProvinceDefinition const* province) {
	if (locked) {
		spdlog::error_s("Cannot remove province from province set - locked!");
		return false;
	}

	if (province == nullptr) {
		spdlog::error_s("Cannot remove province from province set - null province!");
		return false;
	}

	const decltype(provinces)::const_iterator it = std::find(provinces.begin(), provinces.end(), province);
	if (it == provinces.end()) {
		spdlog::warn_s("Cannot remove province {} from province set - not in the set!", *province);
		return false;
	}

	provinces.erase(it);

	return true;
}

void ProvinceSet::lock(bool log) {
	if (locked) {
		spdlog::error_s("Failed to lock province set - already locked!");
	} else {
		locked = true;
		if (log) {
			SPDLOG_INFO("Locked province set with {} provinces", size());
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
		spdlog::error_s("Failed to reserve space for {} items in province set - already locked!", size);
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
