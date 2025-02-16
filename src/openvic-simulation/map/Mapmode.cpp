#include "Mapmode.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"

using namespace OpenVic;
using namespace OpenVic::colour_literals;

Mapmode::Mapmode(
	std::string_view new_identifier,
	index_t new_index,
	colour_func_t new_colour_func,
	std::string_view new_localisation_key,
	bool new_parchment_mapmode_allowed
) : HasIdentifier { new_identifier },
	HasIndex { new_index },
	colour_func { std::move(new_colour_func) },
	localisation_key { new_localisation_key.empty() ? new_identifier : new_localisation_key },
	parchment_mapmode_allowed { new_parchment_mapmode_allowed } {}

const Mapmode Mapmode::ERROR_MAPMODE {
	"mapmode_error", -1, [](
		MapInstance const& map_instance, ProvinceInstance const& province,
		CountryInstance const* player_country, ProvinceInstance const* selected_province
	) -> base_stripe_t {
		return { 0xFFFF0000_argb, colour_argb_t::null() };
	}
};

Mapmode::base_stripe_t Mapmode::get_base_stripe_colours(
	MapInstance const& map_instance, ProvinceInstance const& province,
	CountryInstance const* player_country, ProvinceInstance const* selected_province
) const {
	return colour_func ? colour_func(map_instance, province, player_country, selected_province) : colour_argb_t::null();
}

bool MapmodeManager::add_mapmode(
	std::string_view identifier,
	Mapmode::colour_func_t colour_func,
	std::string_view localisation_key,
	bool parchment_mapmode_allowed
) {
	if (identifier.empty()) {
		Logger::error("Invalid mapmode identifier - empty!");
		return false;
	}
	if (colour_func == nullptr) {
		Logger::error("Mapmode colour function is null for identifier: ", identifier);
		return false;
	}
	return mapmodes.add_item({
		identifier,
		static_cast<Mapmode::index_t>(mapmodes.size()),
		colour_func,
		localisation_key,
		parchment_mapmode_allowed
	});
}

bool MapmodeManager::generate_mapmode_colours(
	MapInstance const& map_instance, Mapmode const* mapmode,
	CountryInstance const* player_country, ProvinceInstance const* selected_province,
	uint8_t* target
) const {
	if (target == nullptr) {
		Logger::error("Mapmode colour target pointer is null!");
		return false;
	}

	bool ret = true;
	if (mapmode == nullptr) {
		mapmode = &Mapmode::ERROR_MAPMODE;
		Logger::error(
			"Trying to generate mapmode colours using null mapmode! Defaulting to \"", mapmode->get_identifier(), "\""
		);
		ret = false;
	}

	Mapmode::base_stripe_t* target_stripes = reinterpret_cast<Mapmode::base_stripe_t*>(target);

	target_stripes[ProvinceDefinition::NULL_INDEX] = colour_argb_t::null();

	if (map_instance.province_instances_are_locked()) {
		for (ProvinceInstance const& province : map_instance.get_province_instances()) {
			target_stripes[province.get_index()] = mapmode->get_base_stripe_colours(
				map_instance, province, player_country, selected_province
			);
		}
	} else {
		for (
			size_t index = ProvinceDefinition::NULL_INDEX + 1;
			index <= map_instance.get_map_definition().get_province_definition_count();
			++index
		) {
			target_stripes[index] = colour_argb_t::null();
		}
	}

	return ret;
}

static constexpr colour_argb_t::value_type ALPHA_VALUE = colour_argb_t::max_value;
/* White default colour, used in mapmodes including political, revolt risk and party loyaly. */
static constexpr colour_argb_t DEFAULT_COLOUR_WHITE = (0xFFFFFF_argb).with_alpha(ALPHA_VALUE);
/* Grey default colour, used in mapmodes including diplomatic, administrative and colonial, recruitment,
 * national focus, RGO, population density, sphere of influence, ranking and migration. */
static constexpr colour_argb_t DEFAULT_COLOUR_GREY = (0x7F7F7F_argb).with_alpha(ALPHA_VALUE);

template<HasGetColour T, typename P>
requires(std::same_as<P, ProvinceDefinition> || std::same_as<P, ProvinceInstance>)
static constexpr auto get_colour_mapmode(T const*(P::*get_item)() const) {
	return [get_item](
		MapInstance const& map_instance, ProvinceInstance const& province,
		CountryInstance const* player_country, ProvinceInstance const* selected_province
	) -> Mapmode::base_stripe_t {
		ProvinceDefinition const& province_definition = province.get_province_definition();

		T const* item = ([&province, &province_definition]() -> P const& {
			if constexpr (std::same_as<P, ProvinceDefinition>) {
				return province_definition;
			} else {
				return province;
			}
		}().*get_item)();

		if (item != nullptr) {
			return colour_argb_t { item->get_colour(), ALPHA_VALUE };
		} else if (!province_definition.is_water()) {
			return DEFAULT_COLOUR_WHITE;
		} else {
			return colour_argb_t::null();
		}
	};
}

template<HasGetColour T>
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

template<HasGetColour T>
static constexpr auto shaded_mapmode(fixed_point_map_t<T const*> const&(ProvinceInstance::*get_map)() const) {
	return [get_map](
		MapInstance const& map_instance, ProvinceInstance const& province,
		CountryInstance const* player_country, ProvinceInstance const* selected_province
	) -> Mapmode::base_stripe_t {
		return shaded_mapmode((province.*get_map)());
	};
}

bool MapmodeManager::setup_mapmodes() {
	if (mapmodes_are_locked()) {
		Logger::error("Cannot setup mapmodes - already locked!");
		return false;
	}

	bool ret = true;

	// Default number of mapmodes
	reserve_mapmodes(22);

	// The order of mapmodes matches their numbering, but is different from the order in which their buttons appear
	ret &= add_mapmode(
		"mapmode_terrain",
		[](
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) -> Mapmode::base_stripe_t {
			return colour_argb_t::null();
		},
		"MAPMODE_1",
		false // Parchment mapmode not allowed
	);
	ret &= add_mapmode("mapmode_political", get_colour_mapmode(&ProvinceInstance::get_owner), "MAPMODE_2");
	ret &= add_mapmode("mapmode_militancy", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_3");
	ret &= add_mapmode("mapmode_diplomatic", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_4");
	ret &= add_mapmode("mapmode_region", get_colour_mapmode(&ProvinceDefinition::get_region), "MAPMODE_5");
	ret &= add_mapmode(
		"mapmode_infrastructure",
		[](
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) -> Mapmode::base_stripe_t {
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
		},
		"MAPMODE_6"
	);
	ret &= add_mapmode("mapmode_colonial", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_7");
	ret &= add_mapmode("mapmode_administrative", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_8");
	ret &= add_mapmode("mapmode_recruitment", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_9");
	ret &= add_mapmode("mapmode_national_focus", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_10");
	ret &= add_mapmode("mapmode_rgo", get_colour_mapmode(&ProvinceInstance::get_rgo_good), "MAPMODE_11");
	ret &= add_mapmode(
		"mapmode_population",
		[](
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) -> Mapmode::base_stripe_t {
			// TODO - explore non-linear scaling to have more variation among non-massive provinces
			// TODO - when selecting a province, only show the population of provinces controlled (or owned?)
			// by the same country, relative to the most populous province in that set of provinces
			if (!province.get_province_definition().is_water()) {
				const colour_argb_t::value_type val = colour_argb_t::colour_traits::component_from_fraction(
					province.get_total_population(), map_instance.get_highest_province_population() + 1, 0.1f, 1.0f
				);
				return colour_argb_t { 0, val, 0, ALPHA_VALUE };
			} else {
				return colour_argb_t::null();
			}
		},
		"MAPMODE_12"
	);
	ret &= add_mapmode("mapmode_culture", shaded_mapmode(&ProvinceInstance::get_culture_distribution), "MAPMODE_13");
	ret &= add_mapmode("mapmode_sphere", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_14");
	ret &= add_mapmode("mapmode_supply", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_15");
	ret &= add_mapmode("mapmode_party_loyalty", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_16");
	ret &= add_mapmode("mapmode_ranking", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_17");
	ret &= add_mapmode("mapmode_migration", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_18");
	ret &= add_mapmode("mapmode_civilisation_level", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_19");
	ret &= add_mapmode("mapmode_relations", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_20");
	ret &= add_mapmode("mapmode_crisis", Mapmode::ERROR_MAPMODE.get_colour_func(), "MAPMODE_21");
	ret &= add_mapmode(
		"mapmode_naval",
		[](
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) -> Mapmode::base_stripe_t {
			ProvinceDefinition const& province_definition = province.get_province_definition();

			if (province_definition.has_port()) {
				return (0xFFFFFF_argb).with_alpha(ALPHA_VALUE);
			} else if (!province_definition.is_water()) {
				return (0x333333_argb).with_alpha(ALPHA_VALUE);
			} else {
				return colour_argb_t::null();
			}
		},
		"MAPMODE_22"
	);

	/*
		*** CUSTOM MAPMODES FOR TESTING ***
		- For these mapmodes to have a button in-game you either need to remove the same number of default mapmodes
		  or add more mapmode buttons (GUIIconButton nodes with path ^".../Menubar/menubar/mapmode_<index>")
	*/
	if constexpr (false) {
		ret &= add_mapmode(
			"mapmode_province",
			[](
				MapInstance const& map_instance, ProvinceInstance const& province,
				CountryInstance const* player_country, ProvinceInstance const* selected_province
			) -> Mapmode::base_stripe_t {
				return colour_argb_t { province.get_province_definition().get_colour(), ALPHA_VALUE };
			}
		);
		ret &= add_mapmode(
			"mapmode_index",
			[](
				MapInstance const& map_instance, ProvinceInstance const& province,
				CountryInstance const* player_country, ProvinceInstance const* selected_province
			) -> Mapmode::base_stripe_t {
				const colour_argb_t::value_type f = colour_argb_t::colour_traits::component_from_fraction(
					province.get_index(), map_instance.get_map_definition().get_province_definition_count() + 1
				);
				return colour_argb_t::fill_as(f).with_alpha(ALPHA_VALUE);
			}
		);
		ret &= add_mapmode("mapmode_religion", shaded_mapmode(&ProvinceInstance::get_religion_distribution));
		ret &= add_mapmode("mapmode_terrain_type", get_colour_mapmode(&ProvinceInstance::get_terrain_type));
		ret &= add_mapmode(
			"mapmode_adjacencies",
			[](
				MapInstance const& map_instance, ProvinceInstance const& province,
				CountryInstance const* player_country, ProvinceInstance const* selected_province
			) -> Mapmode::base_stripe_t {
				if (selected_province != nullptr) {
					ProvinceDefinition const& selected_province_definition = selected_province->get_province_definition();

					if (selected_province == &province) {
						return (0xFFFFFF_argb).with_alpha(ALPHA_VALUE);
					}

					ProvinceDefinition const* province_definition = &province.get_province_definition();

					colour_argb_t base = colour_argb_t::null(), stripe = colour_argb_t::null();
					ProvinceDefinition::adjacency_t const* adj =
						selected_province_definition.get_adjacency_to(province_definition);

					if (adj != nullptr) {
						colour_argb_t::integer_type base_int;
						switch (adj->get_type()) {
							using enum ProvinceDefinition::adjacency_t::type_t;
						case LAND:       base_int = 0x00FF00; break;
						case WATER:      base_int = 0x0000FF; break;
						case COASTAL:    base_int = 0xF9D199; break;
						case IMPASSABLE: base_int = 0x8B4513; break;
						case STRAIT:     base_int = 0x00FFFF; break;
						case CANAL:      base_int = 0x888888; break;
						default:         base_int = 0xFF0000; break;
						}
						base = colour_argb_t::from_integer(base_int).with_alpha(ALPHA_VALUE);
						stripe = base;
					}

					if (selected_province_definition.has_adjacency_going_through(province_definition)) {
						stripe = (0xFFFF00_argb).with_alpha(ALPHA_VALUE);
					}

					return { base, stripe };
				}

				return colour_argb_t::null();
			}
		);
	}

	lock_mapmodes();

	return ret;
}
