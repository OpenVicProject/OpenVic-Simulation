#include "Province.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>

using namespace OpenVic;

Province::Province(index_t new_index, std::string const& new_identifier, colour_t new_colour)
	: HasIdentifier { new_identifier },
	  HasColour { new_colour, false },
	  index { new_index },
	  buildings { "buildings" } {
	assert(index != NULL_INDEX);
}

index_t Province::get_index() const {
	return index;
}

Region* Province::get_region() const {
	return region;
}

bool Province::is_water() const {
	return water;
}

Province::life_rating_t Province::get_life_rating() const {
	return life_rating;
}

return_t Province::add_building(Building&& building) {
	return buildings.add_item(std::move(building));
}

void Province::lock_buildings() {
	buildings.lock(false);
}

void Province::reset_buildings() {
	buildings.reset();
}

Building const* Province::get_building_by_identifier(std::string const& identifier) const {
	return buildings.get_item_by_identifier(identifier);
}

std::vector<Building> const& Province::get_buildings() const {
	return buildings.get_items();
}

return_t Province::expand_building(std::string const& building_type_identifier) {
	Building* building = buildings.get_item_by_identifier(building_type_identifier);
	if (building == nullptr) return FAILURE;
	return building->expand();
}

Good const* Province::get_rgo() const {
	return rgo;
}

std::string Province::to_string() const {
	std::stringstream stream;
	stream << "(#" << std::to_string(index) << ", " << get_identifier() << ", 0x" << colour_to_hex_string() << ")";
	return stream.str();
}

void Province::add_pop(Pop&& pop) {
	pops.push_back(std::move(pop));
}

/* REQUIREMENTS:
 * MAP-65
 */
void Province::update_total_population() {
	total_population = 0;
	for (Pop const& pop : pops) {
		total_population += pop.get_size();
	}
}

Pop::pop_size_t Province::get_total_population() const {
	return total_population;
}

void Province::update_state(Date const& today) {
	for (Building& building : buildings.get_items())
		building.update_state(today);
	update_total_population();
}

void Province::tick(Date const& today) {
	for (Building& building : buildings.get_items())
		building.tick(today);
}
