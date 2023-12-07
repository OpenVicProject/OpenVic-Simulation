#include "GameManager.hpp"

using namespace OpenVic;

GameManager::GameManager(state_updated_func_t state_updated_callback)
	: clock {
		[this]() {
			tick();
		},
		[this]() {
			update_state();
	} }, state_updated { state_updated_callback } {}

void GameManager::set_needs_update() {
	needs_update = true;
}

void GameManager::update_state() {
	if (!needs_update) {
		return;
	}
	Logger::info("Update: ", today);
	map.update_state(today);
	if (state_updated) {
		state_updated();
	}
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

bool GameManager::reset() {
	session_start = time(nullptr);
	clock.reset();
	today = {};
	economy_manager.get_good_manager().reset_to_defaults();
	bool ret = map.reset(economy_manager.get_building_type_manager());
	set_needs_update();
	return ret;
}

bool GameManager::load_bookmark(Bookmark const* new_bookmark) {
	bool ret = reset();
	bookmark = new_bookmark;
	if (bookmark == nullptr) {
		return ret;
	}
	Logger::info("Loading bookmark ", bookmark->get_name(), " with start date ", bookmark->get_date());
	if (!define_manager.in_game_period(bookmark->get_date())) {
		Logger::warning("Bookmark date ", bookmark->get_date(), " is not in the game's time period!");
	}
	today = bookmark->get_date();
	ret &= map.apply_history_to_provinces(history_manager.get_province_manager(), today);
	map.get_state_manager().generate_states(map);
	// TODO - apply country history
	// TODO - apply pop history
	return ret;
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

static constexpr colour_t ALPHA_VALUE = float_to_alpha_value(0.7f);

static constexpr Mapmode::base_stripe_t combine_base_stripe(colour_t base, colour_t stripe) {
	return (static_cast<Mapmode::base_stripe_t>(stripe) << (sizeof(colour_t) * 8)) | base;
}

static constexpr Mapmode::base_stripe_t make_solid_base_stripe(colour_t colour) {
	return combine_base_stripe(colour, colour);
}

static constexpr auto make_solid_base_stripe_func(auto func) {
	return [func](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
		return make_solid_base_stripe(func(map, province));
	};
}

template<std::derived_from<HasColour> T>
static constexpr Mapmode::base_stripe_t get_colour_mapmode(T const* item) {
	return item != nullptr ? make_solid_base_stripe(ALPHA_VALUE | item->get_colour()) : NULL_COLOUR;
}

template<std::derived_from<HasColour> T>
static constexpr auto get_colour_mapmode(T const*(Province::*get_item)() const) {
	return [get_item](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
		T const* item = (province.*get_item)();
		return item != nullptr ? make_solid_base_stripe(ALPHA_VALUE | item->get_colour()) : NULL_COLOUR;
	};
}

template<std::derived_from<HasColour> T>
static constexpr Mapmode::base_stripe_t shaded_mapmode(fixed_point_map_t<T const*> const& map) {
	const std::pair<fixed_point_map_const_iterator_t<T const*>, fixed_point_map_const_iterator_t<T const*>> largest =
		get_largest_two_items(map);
	if (largest.first != map.end()) {
		const colour_t base_colour = ALPHA_VALUE | largest.first->first->get_colour();
		if (largest.second != map.end()) {
			/* If second largest is at least a third... */
			if (largest.second->second * 3 >= get_total(map)) {
				const colour_t stripe_colour = ALPHA_VALUE | largest.second->first->get_colour();
				return combine_base_stripe(base_colour, stripe_colour);
			}
		}
		return make_solid_base_stripe(base_colour);
	}
	return NULL_COLOUR;
}

template<std::derived_from<HasColour> T>
static constexpr auto shaded_mapmode(fixed_point_map_t<T const*> const&(Province::*get_map)() const) {
	return [get_map](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
		return shaded_mapmode((province.*get_map)());
	};
}

bool GameManager::load_hardcoded_defines() {
	bool ret = true;

	using mapmode_t = std::pair<std::string, Mapmode::colour_func_t>;
	const std::vector<mapmode_t> mapmodes {
		{
			"mapmode_terrain",
			[](Map const&, Province const& province) -> Mapmode::base_stripe_t {
				return NULL_COLOUR;
			}
		},
		{
			"mapmode_political", get_colour_mapmode(&Province::get_owner)
		},
		{
			"mapmode_province",
			make_solid_base_stripe_func([](Map const&, Province const& province) -> colour_t {
				return ALPHA_VALUE | province.get_colour();
			})
		},
		{
			"mapmode_region", get_colour_mapmode(&Province::get_region)
		},
		{
			"mapmode_index",
			make_solid_base_stripe_func([](Map const& map, Province const& province) -> colour_t {
				const colour_t f = fraction_to_colour_byte(province.get_index(), map.get_province_count() + 1);
				return ALPHA_VALUE | (f << 16) | (f << 8) | f;
			})
		},
		{
			"mapmode_terrain_type", get_colour_mapmode(&Province::get_terrain_type)
		},
		{
			"mapmode_rgo", get_colour_mapmode(&Province::get_rgo)
		},
		{
			"mapmode_infrastructure",
			make_solid_base_stripe_func([](Map const& map, Province const& province) -> colour_t {
				BuildingInstance const* railroad = province.get_building_by_identifier("railroad");
				if (railroad != nullptr) {
					colour_t val = fraction_to_colour_byte(railroad->get_level(),
						railroad->get_building_type().get_max_level() + 1, 0.5f, 1.0f);
					switch (railroad->get_expansion_state()) {
					case BuildingInstance::ExpansionState::CannotExpand:
						val <<= 16;
						break;
					case BuildingInstance::ExpansionState::CanExpand:
						break;
					default:
						val <<= 8;
						break;
					}
					return ALPHA_VALUE | val;
				}
				return NULL_COLOUR;
			})
		},
		{
			"mapmode_population",
			make_solid_base_stripe_func([](Map const& map, Province const& province) -> colour_t {
				// TODO - explore non-linear scaling to have more variation among non-massive provinces
				// TODO - when selecting a province, only show the population of provinces controlled (or owned?)
				// by the same country, relative to the most populous province in that set of provinces
				return ALPHA_VALUE | (fraction_to_colour_byte(
					province.get_total_population(), map.get_highest_province_population() + 1, 0.1f, 1.0f
				) << 8);
			})
		},
		{
			"mapmode_culture", shaded_mapmode(&Province::get_culture_distribution)
		},
		{
			"mapmode_religion", shaded_mapmode(&Province::get_religion_distribution)
		}
	};

	for (mapmode_t const& mapmode : mapmodes) {
		ret &= map.add_mapmode(mapmode.first, mapmode.second);
	}
	map.lock_mapmodes();

	return ret;
}
