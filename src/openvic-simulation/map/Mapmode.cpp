#include "Mapmode.hpp"

#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/BuildingTypeManager.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"

using namespace OpenVic;
using namespace OpenVic::colour_literals;

Mapmode::Mapmode(
	std::string_view new_identifier,
	index_t new_index,
	colour_func_t new_colour_func,
	std::string_view new_localisation_key,
	bool new_is_parchment_mapmode_allowed
) : HasIdentifier { new_identifier },
	HasIndex { new_index },
	colour_func { std::move(new_colour_func) },
	localisation_key { new_localisation_key.empty() ? new_identifier : new_localisation_key },
	is_parchment_mapmode_allowed { new_is_parchment_mapmode_allowed } {}

const Mapmode Mapmode::ERROR_MAPMODE {
	"mapmode_error", index_t { std::numeric_limits<std::size_t>::max() }, [](
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
