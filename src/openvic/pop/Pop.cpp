#include "Pop.hpp"

#include <cassert>

#include "../utility/Logger.hpp"

using namespace OpenVic;

Pop::Pop(PopType const& new_type, Religion const& new_religion)
	: type { new_type }, religion { new_religion }, size { 1 } {
	assert(size > 0);
}

PopType const& Pop::get_type() const {
	return type;
}

Religion const& Pop::get_religion() const {
	return religion;
}

Pop::pop_size_t Pop::get_size() const {
	return size;
}

PopType::PopType(std::string const& new_identifier, colour_t new_colour, strata_t new_strata, sprite_t new_sprite,
	Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
	bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan)
	: HasIdentifier { new_identifier },
	  HasColour { new_colour, true },
	  strata { new_strata },
	  sprite { new_sprite },
	  max_size { new_max_size },
	  merge_max_size { new_merge_max_size },
	  state_capital_only { new_state_capital_only },
	  demote_migrant { new_demote_migrant },
	  is_artisan { new_is_artisan } {
	assert(sprite > 0);
	assert(max_size > 0);
	assert(merge_max_size > 0);
}

PopType::sprite_t PopType::get_sprite() const {
	return sprite;
}

Pop::pop_size_t PopType::get_max_size() const {
	return max_size;
}

Pop::pop_size_t PopType::get_merge_max_size() const {
	return merge_max_size;
}

bool PopType::get_state_capital_only() const {
	return state_capital_only;
}

bool PopType::get_demote_migrant() const {
	return demote_migrant;
}

bool PopType::get_is_artisan() const {
	return is_artisan;
}

PopTypeManager::PopTypeManager() : pop_types { "pop types" } {}

return_t PopTypeManager::add_pop_type(std::string const& identifier, colour_t colour, PopType::strata_t strata, PopType::sprite_t sprite, Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size, bool state_capital_only, bool demote_migrant, bool is_artisan) {
	if (identifier.empty()) {
		Logger::error("Invalid pop type identifier - empty!");
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid pop type colour for ", identifier, ": ", PopType::colour_to_hex_string(colour));
		return FAILURE;
	}
	if (sprite <= 0) {
		Logger::error("Invalid pop type sprite index for ", identifier, ": ", sprite);
		return FAILURE;
	}
	if (max_size <= 0) {
		Logger::error("Invalid pop type max size for ", identifier, ": ", max_size);
		return FAILURE;
	}
	if (merge_max_size <= 0) {
		Logger::error("Invalid pop type merge max size for ", identifier, ": ", merge_max_size);
		return FAILURE;
	}
	return pop_types.add_item({ identifier, colour, strata, sprite, max_size, merge_max_size, state_capital_only, demote_migrant, is_artisan });
}

void PopTypeManager::lock_pop_types() {
	pop_types.lock();
}

PopType const* PopTypeManager::get_pop_type_by_identifier(std::string const& identifier) const {
	return pop_types.get_item_by_identifier(identifier);
}
