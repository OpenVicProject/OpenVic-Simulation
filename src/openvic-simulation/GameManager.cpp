#include "GameManager.hpp"

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;
using namespace OpenVic::colour_literals;

GameManager::GameManager(
	gamestate_updated_func_t gamestate_updated_callback, SimulationClock::state_changed_function_t clock_state_changed_callback
) : simulation_clock {
		std::bind(&GameManager::tick, this), std::bind(&GameManager::update_gamestate, this), clock_state_changed_callback
	}, gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} },
	session_start { 0 }, today {}, gamestate_needs_update { false }, currently_updating_gamestate { false } {}

void GameManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		Logger::error("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void GameManager::update_gamestate() {
	if (!gamestate_needs_update) {
		return;
	}
	currently_updating_gamestate = true;
	Logger::info("Update: ", today);
	map.update_gamestate(today);
	gamestate_updated();
	gamestate_needs_update = false;
	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void GameManager::tick() {
	today++;
	Logger::info("Tick: ", today);
	map.tick(today);
	set_gamestate_needs_update();
}

bool GameManager::reset() {
	session_start = time(nullptr);
	simulation_clock.reset();
	today = {};
	economy_manager.get_good_manager().reset_to_defaults();
	bool ret = map.reset(economy_manager.get_building_type_manager());
	set_gamestate_needs_update();
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
	return ret;
}

bool GameManager::expand_selected_province_building(size_t building_index) {
	set_gamestate_needs_update();
	Province* province = map.get_selected_province();
	if (province == nullptr) {
		Logger::error("Cannot expand building index ", building_index, " - no province selected!");
		return false;
	}
	if (building_index < 0) {
		Logger::error("Invalid building index ", building_index, " while trying to expand in province ", province);
		return false;
	}
	return province->expand_building(building_index);
}

static constexpr colour_argb_t::value_type ALPHA_VALUE = colour_argb_t::colour_traits::alpha_from_float(0.7f);

template<IsColour ColourT = colour_t, std::derived_from<_HasColour<ColourT>> T>
static constexpr auto get_colour_mapmode(T const*(Province::*get_item)() const) {
	return [get_item](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
		T const* item = (province.*get_item)();
		return item != nullptr ? colour_argb_t { item->get_colour(), ALPHA_VALUE } : colour_argb_t::null();
	};
}

template<IsColour ColourT = colour_t, std::derived_from<_HasColour<ColourT>> T>
static constexpr Mapmode::base_stripe_t shaded_mapmode(fixed_point_map_t<T const*> const& map) {
	const std::pair<fixed_point_map_const_iterator_t<T const*>, fixed_point_map_const_iterator_t<T const*>> largest =
		get_largest_two_items(map);
	if (largest.first != map.end()) {
		const colour_argb_t base_colour = colour_argb_t { largest.first->first->get_colour(), ALPHA_VALUE };
		if (largest.second != map.end()) {
			/* If second largest is at least a third... */
			if (largest.second->second * 3 >= get_total(map)) {
				const colour_argb_t stripe_colour = colour_argb_t { largest.second->first->get_colour(), ALPHA_VALUE };
				return { base_colour, stripe_colour };
			}
		}
		return base_colour;
	}
	return colour_argb_t::null();
}

template<IsColour ColourT = colour_t, std::derived_from<_HasColour<ColourT>> T>
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
				return colour_argb_t::null();
			}
		},
		{
			"mapmode_political", get_colour_mapmode(&Province::get_owner)
		},
		{
			"mapmode_province",
			[](Map const&, Province const& province) -> Mapmode::base_stripe_t {
				return colour_argb_t { province.get_colour(), ALPHA_VALUE };
			}
		},
		{
			"mapmode_region", get_colour_mapmode(&Province::get_region)
		},
		{
			"mapmode_index",
			[](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
				const colour_argb_t::value_type f =
					colour_argb_t::colour_traits::component_from_fraction(province.get_index(), map.get_province_count() + 1);
				return colour_argb_t::fill_as(f).with_alpha(ALPHA_VALUE);
			}
		},
		{
			"mapmode_terrain_type", get_colour_mapmode(&Province::get_terrain_type)
		},
		{
			"mapmode_rgo", get_colour_mapmode(&Province::get_rgo)
		},
		{
			"mapmode_infrastructure",
			[](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
				BuildingInstance const* railroad = province.get_building_by_identifier("railroad");
				if (railroad != nullptr) {
					const colour_argb_t::value_type val = colour_argb_t::colour_traits::component_from_fraction(
						railroad->get_level(), railroad->get_building_type().get_max_level() + 1, 0.5f, 1.0f
					);
					switch (railroad->get_expansion_state()) {
					case BuildingInstance::ExpansionState::CannotExpand:
						return colour_argb_t { val, 0, 0, ALPHA_VALUE };
					case BuildingInstance::ExpansionState::CanExpand:
						return colour_argb_t { 0, 0, val, ALPHA_VALUE };
					default:
						return colour_argb_t { 0, val, 0, ALPHA_VALUE };
					}
				}
				return colour_argb_t::null();
			}
		},
		{
			"mapmode_population",
			[](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
				// TODO - explore non-linear scaling to have more variation among non-massive provinces
				// TODO - when selecting a province, only show the population of provinces controlled (or owned?)
				// by the same country, relative to the most populous province in that set of provinces
				const colour_argb_t::value_type val = colour_argb_t::colour_traits::component_from_fraction(
					province.get_total_population(), map.get_highest_province_population() + 1, 0.1f, 1.0f
				);
				return colour_argb_t { 0, val, 0, ALPHA_VALUE };
			}
		},
		{
			"mapmode_culture", shaded_mapmode(&Province::get_culture_distribution)
		},
		{
			"mapmode_religion", shaded_mapmode(&Province::get_religion_distribution)
		},
		{
			"mapmode_adjacencies", [](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
				Province const* selected_province = map.get_selected_province();
				if (selected_province != nullptr) {
					if (selected_province == &province) {
						return (0xFFFFFF_argb).with_alpha(ALPHA_VALUE);
					}
					colour_argb_t base = colour_argb_t::null(), stripe = colour_argb_t::null();
					Province::adjacency_t const* adj = selected_province->get_adjacency_to(&province);
					if (adj != nullptr) {
						colour_argb_t::integer_type base_int;
						switch (adj->get_type()) {
							using enum Province::adjacency_t::type_t;
						case LAND:		 base_int = 0x00FF00; break;
						case WATER:		 base_int = 0x0000FF; break;
						case COASTAL:	 base_int = 0xF9D199; break;
						case IMPASSABLE: base_int = 0x8B4513; break;
						case STRAIT:	 base_int = 0x00FFFF; break;
						case CANAL:		 base_int = 0x888888; break;
						default:		 base_int = 0xFF0000; break;
						}
						base = colour_argb_t::from_integer(base_int).with_alpha(ALPHA_VALUE);
						stripe = base;
					}
					if (selected_province->has_adjacency_going_through(&province)) {
						stripe = (0xFFFF00_argb).with_alpha(ALPHA_VALUE);
					}

					return { base, stripe };
				}
				return colour_argb_t::null();
			}
		},
		{
			"mapmode_port", [](Map const& map, Province const& province) -> Mapmode::base_stripe_t {
				return province.has_port() ? (0xFFFFFF_argb).with_alpha(ALPHA_VALUE) : colour_argb_t::null();
			}
		}
	};

	for (mapmode_t const& mapmode : mapmodes) {
		ret &= map.add_mapmode(mapmode.first, mapmode.second);
	}
	map.lock_mapmodes();

	return ret;
}
