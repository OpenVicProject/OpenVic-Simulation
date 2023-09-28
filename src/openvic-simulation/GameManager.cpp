#include "GameManager.hpp"

using namespace OpenVic;

GameManager::GameManager(state_updated_func_t state_updated_callback)
	: clock { [this]() { tick(); }, [this]() { update_state(); } },
	  state_updated { state_updated_callback } {}

Map& GameManager::get_map() {
	return map;
}

Map const& GameManager::get_map() const {
	return map;
}

BuildingManager& GameManager::get_building_manager() {
	return building_manager;
}

BuildingManager const& GameManager::get_building_manager() const {
	return building_manager;
}

GoodManager& GameManager::get_good_manager() {
	return good_manager;
}

GoodManager const& GameManager::get_good_manager() const {
	return good_manager;
}

PopManager& GameManager::get_pop_manager() {
	return pop_manager;
}

PopManager const& GameManager::get_pop_manager() const {
	return pop_manager;
}

IdeologyManager& GameManager::get_ideology_manager() {
	return ideology_manager;
}

IdeologyManager const& GameManager::get_ideology_manager() const {
	return ideology_manager;
}

IssueManager& GameManager::get_issue_manager() {
	return issue_manager;
}

IssueManager const& GameManager::get_issue_manager() const {
	return issue_manager;
}

ProductionTypeManager& GameManager::get_production_type_manager() {
	return production_type_manager;
}

ProductionTypeManager const& GameManager::get_production_type_manager() const {
	return production_type_manager;
}

UnitManager& GameManager::get_unit_manager() {
	return unit_manager;
}

UnitManager const& GameManager::get_unit_manager() const {
	return unit_manager;
}

ModifierManager& GameManager::get_modifier_manager() {
	return modifier_manager;
}

ModifierManager const& GameManager::get_modifier_manager() const{
	return modifier_manager;
}

GameAdvancementHook& GameManager::get_clock() {
	return clock;
}

GameAdvancementHook const& GameManager::get_clock() const {
	return clock;
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

bool GameManager::expand_building(Province::index_t province_index, std::string_view building_type_identifier) {
	set_needs_update();
	Province* province = map.get_province_by_index(province_index);
	if (province == nullptr) {
		Logger::error("Invalid province index ", province_index, " while trying to expand building ", building_type_identifier);
		return false;
	}
	return province->expand_building(building_type_identifier);
}

static constexpr colour_t LOW_ALPHA_VALUE = float_to_alpha_value(0.4f);
static constexpr colour_t HIGH_ALPHA_VALUE = float_to_alpha_value(0.7f);

static colour_t default_colour(Province const& province) {
	/* Nice looking colours to blend with the terrain textures */
	static constexpr colour_t LAND_COLOUR = 0x0D7017;
	static constexpr colour_t WATER_COLOUR = 0x4287F5;
	return LOW_ALPHA_VALUE | (province.get_water() ? WATER_COLOUR : LAND_COLOUR);
}

bool GameManager::load_hardcoded_defines() {
	bool ret = true;

	using mapmode_t = std::pair<std::string, Mapmode::colour_func_t>;
	const std::vector<mapmode_t> mapmodes {
		{ "mapmode_terrain",
			[](Map const&, Province const& province) -> colour_t {
				return default_colour(province);
			} },
		{ "mapmode_province",
			[](Map const&, Province const& province) -> colour_t {
				return HIGH_ALPHA_VALUE | province.get_colour();
			} },
		{ "mapmode_region",
			[](Map const&, Province const& province) -> colour_t {
				Region const* region = province.get_region();
				return region != nullptr ? HIGH_ALPHA_VALUE | region->get_colour() : default_colour(province);
			} },
		{ "mapmode_index",
			[](Map const& map, Province const& province) -> colour_t {
				const colour_t f = fraction_to_colour_byte(province.get_index(), map.get_province_count() + 1);
				return HIGH_ALPHA_VALUE | (f << 16) | (f << 8) | f;
			} },
		{ "mapmode_terrain_type",
			[](Map const& map, Province const& province) -> colour_t {
				TerrainType const* terrarin_type = province.get_terrain_type();
				return terrarin_type != nullptr ? HIGH_ALPHA_VALUE | terrarin_type->get_colour() : default_colour(province);
			} },
		{ "mapmode_rgo",
			[](Map const& map, Province const& province) -> colour_t {
				Good const* rgo = province.get_rgo();
				return rgo != nullptr ? HIGH_ALPHA_VALUE | rgo->get_colour() : default_colour(province);
			} },
		{ "mapmode_infrastructure",
			[](Map const& map, Province const& province) -> colour_t {
				BuildingInstance const* railroad = province.get_building_by_identifier("building_railroad");
				if (railroad != nullptr) {
					colour_t val = fraction_to_colour_byte(railroad->get_current_level(), railroad->get_building().get_max_level() + 1, 0.5f, 1.0f);
					switch (railroad->get_expansion_state()) {
						case ExpansionState::CannotExpand: val <<= 16; break;
						case ExpansionState::CanExpand: break;
						default: val <<= 8; break;
					}
					return HIGH_ALPHA_VALUE | val;
				}
				return default_colour(province);
			} },
		{ "mapmode_population",
			[](Map const& map, Province const& province) -> colour_t {
				return HIGH_ALPHA_VALUE | (fraction_to_colour_byte(province.get_total_population(), map.get_highest_province_population() + 1, 0.1f, 1.0f) << 8);
			} },
		{ "mapmode_culture",
			[](Map const& map, Province const& province) -> colour_t {
				HasIdentifierAndColour const* largest = get_largest_item(province.get_culture_distribution()).first;
				return largest != nullptr ? HIGH_ALPHA_VALUE | largest->get_colour() : default_colour(province);
			} },
		{ "mapmode_religion",
			[](Map const& map, Province const& province) -> colour_t {
				HasIdentifierAndColour const* largest = get_largest_item(province.get_religion_distribution()).first;
				return largest != nullptr ? HIGH_ALPHA_VALUE | largest->get_colour() : default_colour(province);
			} }
	};
	for (mapmode_t const& mapmode : mapmodes)
		ret &= map.add_mapmode(mapmode.first, mapmode.second);
	map.lock_mapmodes();

	return ret;
}
