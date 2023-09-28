#include "Province.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Province::Province(std::string_view new_identifier, colour_t new_colour, index_t new_index)
	: HasIdentifierAndColour { new_identifier, new_colour, false, false },
	  index { new_index }, buildings { "buildings", false } {
	assert(index != NULL_INDEX);
}

Province::index_t Province::get_index() const {
	return index;
}

Region* Province::get_region() const {
	return region;
}

bool Province::get_has_region() const {
	return has_region;
}

bool Province::get_water() const {
	return water;
}

TerrainType const* Province::get_terrain_type() const {
	return terrain_type;
}

Province::life_rating_t Province::get_life_rating() const {
	return life_rating;
}

bool Province::load_positions(BuildingManager const& building_manager, ast::NodeCPtr root) {
	// TODO - implement province position loading
	// (root is the dictionary after the province identifier)
	return true;
}

bool Province::add_building(BuildingInstance&& building_instance) {
	return buildings.add_item(std::move(building_instance));
}

void Province::reset_buildings() {
	buildings.reset();
}

bool Province::expand_building(std::string_view building_type_identifier) {
	BuildingInstance* building = buildings.get_item_by_identifier(building_type_identifier);
	if (building == nullptr) return false;
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

bool Province::load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(
		pops,
		[this, &pop_manager](std::string_view pop_type_identifier, ast::NodeCPtr pop_node) -> bool {
			return pop_manager.load_pop_into_province(*this, pop_type_identifier, pop_node);
		}
	)(root);
}

bool Province::add_pop(Pop&& pop) {
	if (!get_water()) {
		pops.push_back(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

void Province::clear_pops() {
	pops.clear();
}

size_t Province::get_pop_count() const {
	return pops.size();
}

std::vector<Pop> const& Province::get_pops() const {
	return pops;
}

Pop::pop_size_t Province::get_total_population() const {
	return total_population;
}

distribution_t const& Province::get_pop_type_distribution() const {
	return pop_types;
}

distribution_t const& Province::get_culture_distribution() const {
	return cultures;
}

distribution_t const& Province::get_religion_distribution() const {
	return religions;
}

/* REQUIREMENTS:
 * MAP-65, MAP-68, MAP-70, MAP-234
 */
void Province::update_pops() {
	total_population = 0;
	pop_types.clear();
	cultures.clear();
	religions.clear();
	for (Pop const& pop : pops) {
		total_population += pop.get_size();
		pop_types[&pop.get_type()] += pop.get_size();
		cultures[&pop.get_culture()] += pop.get_size();
		religions[&pop.get_religion()] += pop.get_size();
	}
}

void Province::update_state(Date const& today) {
	for (BuildingInstance& building : buildings.get_items())
		building.update_state(today);
	update_pops();
}

void Province::tick(Date const& today) {
	for (BuildingInstance& building : buildings.get_items())
		building.tick(today);
}

Province::adjacency_t::adjacency_t(Province const* province, distance_t distance, flags_t flags)
	: province { province }, distance { distance }, flags { flags } {
	assert(province != nullptr);
}

Province::distance_t Province::adjacency_t::get_distance() const {
	return distance;
}

Province::flags_t Province::adjacency_t::get_flags() const {
	return flags;
}

bool Province::is_adjacent_to(Province const* province) {
	for (adjacency_t adj : adjacencies)
		if (adj.province == province)
			return true;
	return false;
}

bool Province::add_adjacency(Province const* province, distance_t distance, flags_t flags) {
	if (province == nullptr) {
		Logger::error("Tried to create null adjacency province for province ", get_identifier(), "!");
		return false;
	}

	if (is_adjacent_to(province))
		return false;
	adjacencies.push_back({ province, distance, flags });
	return true;
}

std::vector<Province::adjacency_t> const& Province::get_adjacencies() const {
	return adjacencies;
}

void Province::_set_terrain_type(TerrainType const* type) {
		terrain_type = type;
}
