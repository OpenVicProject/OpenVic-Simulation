#include "Mapmode.hpp"

#include <cassert>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;
using namespace OpenVic::colour_literals;

Mapmode::Mapmode(
	std::string_view new_identifier, index_t new_index, colour_func_t new_colour_func
) : HasIdentifier { new_identifier }, index { new_index }, colour_func { new_colour_func } {
	assert(colour_func != nullptr);
}

const Mapmode Mapmode::ERROR_MAPMODE {
	"mapmode_error", 0, [](MapInstance const&, ProvinceInstance const& province) -> base_stripe_t {
		return { 0xFFFF0000_argb, colour_argb_t::null() };
	}
};

Mapmode::base_stripe_t Mapmode::get_base_stripe_colours(
	MapInstance const& map_instance, ProvinceInstance const& province
) const {
	return colour_func ? colour_func(map_instance, province) : colour_argb_t::null();
}

bool MapmodeManager::add_mapmode(std::string_view identifier, Mapmode::colour_func_t colour_func) {
	if (identifier.empty()) {
		Logger::error("Invalid mapmode identifier - empty!");
		return false;
	}
	if (colour_func == nullptr) {
		Logger::error("Mapmode colour function is null for identifier: ", identifier);
		return false;
	}
	return mapmodes.add_item({ identifier, mapmodes.size(), colour_func });
}

bool MapmodeManager::generate_mapmode_colours(MapInstance const& map_instance, Mapmode::index_t index, uint8_t* target) const {
	if (target == nullptr) {
		Logger::error("Mapmode colour target pointer is null!");
		return false;
	}

	bool ret = true;
	Mapmode const* mapmode = get_mapmode_by_index(index);
	if (mapmode == nullptr) {
		// Not an error if mapmodes haven't yet been loaded,
		// e.g. if we want to allocate the province colour
		// texture before mapmodes are loaded.
		if (!(mapmodes_empty() && index == 0)) {
			Logger::error("Invalid mapmode index: ", index);
			ret = false;
		}
		mapmode = &Mapmode::ERROR_MAPMODE;
	}

	Mapmode::base_stripe_t* target_stripes = reinterpret_cast<Mapmode::base_stripe_t*>(target);

	target_stripes[ProvinceDefinition::NULL_INDEX] = colour_argb_t::null();

	if (map_instance.province_instances_are_locked()) {
		for (ProvinceInstance const& province : map_instance.get_province_instances()) {
			target_stripes[province.get_province_definition().get_index()] =
				mapmode->get_base_stripe_colours(map_instance, province);
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

template<utility::is_derived_from_specialization_of<_HasColour> T, typename P>
requires(std::same_as<P, ProvinceDefinition> || std::same_as<P, ProvinceInstance>)
static constexpr auto get_colour_mapmode(T const*(P::*get_item)() const) {
	return [get_item](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
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

template<utility::is_derived_from_specialization_of<_HasColour> T>
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

template<utility::is_derived_from_specialization_of<_HasColour> T>
static constexpr auto shaded_mapmode(fixed_point_map_t<T const*> const&(ProvinceInstance::*get_map)() const) {
	return [get_map](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
		return shaded_mapmode((province.*get_map)());
	};
}

bool MapmodeManager::setup_mapmodes() {
	bool ret = true;

	using mapmode_definition_t = std::pair<std::string, Mapmode::colour_func_t>;
	const std::vector<mapmode_definition_t> mapmode_definitions {
		{
			"mapmode_terrain",
			[](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
				return colour_argb_t::null();
			}
		},
		{
			"mapmode_political", get_colour_mapmode(&ProvinceInstance::get_owner)
		},
		{
			/* TEST MAPMODE, TO BE REMOVED */
			"mapmode_province",
			[](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
				return colour_argb_t { province.get_province_definition().get_colour(), ALPHA_VALUE };
			}
		},
		{
			"mapmode_region", get_colour_mapmode(&ProvinceDefinition::get_region)
		},
		{
			/* TEST MAPMODE, TO BE REMOVED */
			"mapmode_index",
			[](MapInstance const& map_instance, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
				const colour_argb_t::value_type f = colour_argb_t::colour_traits::component_from_fraction(
					province.get_province_definition().get_index(),
					map_instance.get_map_definition().get_province_definition_count() + 1
				);
				return colour_argb_t::fill_as(f).with_alpha(ALPHA_VALUE);
			}
		},
		{
			/* Non-vanilla mapmode, still of use in game. */
			"mapmode_terrain_type", get_colour_mapmode(&ProvinceInstance::get_terrain_type)
		},
		{
			"mapmode_rgo", get_colour_mapmode(&ProvinceInstance::get_rgo)
		},
		{
			"mapmode_infrastructure",
			[](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
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
			[](MapInstance const& map_instance, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
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
			}
		},
		{
			"mapmode_culture", shaded_mapmode(&ProvinceInstance::get_culture_distribution)
		},
		{
			/* Non-vanilla mapmode, still of use in game. */
			"mapmode_religion", shaded_mapmode(&ProvinceInstance::get_religion_distribution)
		},
		{
			/* TEST MAPMODE, TO BE REMOVED */
			"mapmode_adjacencies",
			[](MapInstance const& map_instance, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
				ProvinceInstance const* selected_province = map_instance.get_selected_province();

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
		},
		{
			"mapmode_port", [](MapInstance const&, ProvinceInstance const& province) -> Mapmode::base_stripe_t {
				ProvinceDefinition const& province_definition = province.get_province_definition();

				if (province_definition.has_port()) {
					return (0xFFFFFF_argb).with_alpha(ALPHA_VALUE);
				} else if (!province_definition.is_water()) {
					return (0x333333_argb).with_alpha(ALPHA_VALUE);
				} else {
					return colour_argb_t::null();
				}
			}
		}
	};

	mapmodes.reset();
	reserve_mapmodes(mapmode_definitions.size());

	for (mapmode_definition_t const& mapmode : mapmode_definitions) {
		ret &= add_mapmode(mapmode.first, mapmode.second);
	}

	lock_mapmodes();

	return ret;
}
