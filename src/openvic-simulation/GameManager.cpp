#include "GameManager.hpp"

#include "openvic-simulation/utility/Logger.hpp"
#include "units/Unit.hpp"

using namespace OpenVic;

GameManager::GameManager(state_updated_func_t state_updated_callback)
	: unit_manager { good_manager },
	  clock { [this]() { tick(); }, [this]() { update_state(); } },
	  state_updated { state_updated_callback } {}
	  
Map* GameManager::get_map() {
	return &map;
}

BuildingManager* GameManager::get_building_manager() {
	return &building_manager;
}

GoodManager* GameManager::get_good_manager() {
	return &good_manager;
}

PopManager* GameManager::get_pop_manager() {
	return &pop_manager;
}

GameAdvancementHook* GameManager::get_game_advancement_hook() {
	return &clock;
}

Map* GameManager::get_map() {
	return &map;
}

BuildingManager* GameManager::get_building_manager() {
	return &building_manager;
}

GoodManager* GameManager::get_good_manager() {
	return &good_manager;
}

PopManager* GameManager::get_pop_manager() {
	return &pop_manager;
}

GameAdvancementHook* GameManager::get_game_advancement_hook() {
	return &clock;
}

void GameManager::set_needs_update() {
	needs_update = true;
}

void GameManager::update_state() {
	if (!needs_update) return;
	Logger::info("Update: ", today);
	map.update_state(today);
	if (state_updated) state_updated();
	needs_update = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void GameManager::tick() {
	today++;
	Logger::info("Tick: ", today);
	map.tick(today);
	set_needs_update();
}

bool GameManager::setup() {
	session_start = time(nullptr);
	clock.reset();
	today = { 1836 };
	good_manager.reset_to_defaults();
	bool ret = map.setup(good_manager, building_manager, pop_manager);
	set_needs_update();
	return ret;
}

Date const& GameManager::get_today() const {
	return today;
}

bool GameManager::expand_building(Province::index_t province_index, const std::string_view building_type_identifier) {
	set_needs_update();
	Province* province = map.get_province_by_index(province_index);
	if (province == nullptr) {
		Logger::error("Invalid province index ", province_index, " while trying to expand building ", building_type_identifier);
		return false;
	}
	return province->expand_building(building_type_identifier);
}

bool GameManager::load_hardcoded_defines() {
	bool ret = true;

	static constexpr colour_t LOW_ALPHA_VALUE = float_to_alpha_value(0.4f);
	static constexpr colour_t HIGH_ALPHA_VALUE = float_to_alpha_value(0.7f);
	using mapmode_t = std::pair<std::string, Mapmode::colour_func_t>;
	const std::vector<mapmode_t> mapmodes {
		{ "mapmode_terrain",
			[](Map const&, Province const& province) -> colour_t {
				return LOW_ALPHA_VALUE | (province.get_water() ? 0x4287F5 : 0x0D7017);
			} },
		{ "mapmode_province",
			[](Map const&, Province const& province) -> colour_t {
				return HIGH_ALPHA_VALUE | province.get_colour();
			} },
		{ "mapmode_region",
			[](Map const&, Province const& province) -> colour_t {
				Region const* region = province.get_region();
				if (region != nullptr) return HIGH_ALPHA_VALUE | region->get_colour();
				return NULL_COLOUR;
			} },
		{ "mapmode_index",
			[](Map const& map, Province const& province) -> colour_t {
				const colour_t f = fraction_to_colour_byte(province.get_index(), map.get_province_count() + 1);
				return HIGH_ALPHA_VALUE | (f << 16) | (f << 8) | f;
			} },
		{ "mapmode_rgo",
			[](Map const& map, Province const& province) -> colour_t {
				Good const* rgo = province.get_rgo();
				if (rgo != nullptr) return HIGH_ALPHA_VALUE | rgo->get_colour();
				return NULL_COLOUR;
			} },
		{ "mapmode_infrastructure",
			[](Map const& map, Province const& province) -> colour_t {
				Building const* railroad = province.get_building_by_identifier("building_railroad");
				if (railroad != nullptr) {
					colour_t val = fraction_to_colour_byte(railroad->get_level(), railroad->get_type().get_max_level() + 1, 0.5f, 1.0f);
					switch (railroad->get_expansion_state()) {
						case Building::ExpansionState::CannotExpand: val <<= 16; break;
						case Building::ExpansionState::CanExpand: break;
						default: val <<= 8; break;
					}
					return HIGH_ALPHA_VALUE | val;
				}
				return NULL_COLOUR;
			} },
		{ "mapmode_population",
			[](Map const& map, Province const& province) -> colour_t {
				return HIGH_ALPHA_VALUE | (fraction_to_colour_byte(province.get_total_population(), map.get_highest_province_population() + 1, 0.1f, 1.0f) << 8);
			} },
		{ "mapmode_culture",
			[](Map const& map, Province const& province) -> colour_t {
				HasIdentifierAndColour const* largest = get_largest_item(province.get_culture_distribution()).first;
				return largest != nullptr ? HIGH_ALPHA_VALUE | largest->get_colour() : NULL_COLOUR;
			} },
		{ "mapmode_religion",
			[](Map const& map, Province const& province) -> colour_t {
				HasIdentifierAndColour const* largest = get_largest_item(province.get_religion_distribution()).first;
				return largest != nullptr ? HIGH_ALPHA_VALUE | largest->get_colour() : NULL_COLOUR;
			} }
	};
	for (mapmode_t const& mapmode : mapmodes)
		ret &= map.add_mapmode(mapmode.first, mapmode.second);
	map.lock_mapmodes();

	using building_type_t = std::tuple<std::string, Building::level_t, Timespan>;
	const std::vector<building_type_t> building_types {
		{ "building_fort", 4, 8 }, { "building_naval_base", 6, 15 }, { "building_railroad", 5, 10 }	// Move this to building.hpp
	};
	for (building_type_t const& type : building_types)
		ret &= building_manager.add_building_type(std::get<0>(type), std::get<1>(type), std::get<2>(type));
	building_manager.lock_building_types();

	return ret;
}
