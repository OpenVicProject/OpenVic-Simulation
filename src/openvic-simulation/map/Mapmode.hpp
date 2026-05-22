#pragma once

#include <function2/function2.hpp>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct MapInstance;
	struct ProvinceInstance;
	struct CountryInstance;

	struct Mapmode : HasIdentifier, HasIndex<Mapmode, map_mode_index_t> {
		/* Bottom 32 bits are the base colour, top 32 are the stripe colour, both in ARGB format with the alpha channels
		 * controlling interpolation with the terrain colour (0 = all terrain, 255 = all corresponding RGB) */
		struct base_stripe_t {
			colour_argb_t base_colour;
			colour_argb_t stripe_colour;
			constexpr base_stripe_t(colour_argb_t base, colour_argb_t stripe)
				: base_colour { base }, stripe_colour { stripe } {}
			constexpr base_stripe_t(colour_argb_t both) : base_stripe_t { both, both } {}
		};
		using colour_func_t = fu2::function<
			base_stripe_t(MapInstance const&, ProvinceInstance const&, CountryInstance const*, ProvinceInstance const*) const
		>;

	private:
		// Not const so they don't have to be copied when the Mapmode is moved
		colour_func_t PROPERTY(colour_func);
		memory::string PROPERTY(localisation_key);

	public:
		static const Mapmode ERROR_MAPMODE;
		const bool is_parchment_mapmode_allowed;

		Mapmode(
			std::string_view new_identifier,
			index_t new_index,
			colour_func_t new_colour_func,
			std::string_view new_localisation_key = {},
			bool new_is_parchment_mapmode_allowed = true
		);
		Mapmode(Mapmode&&) = default;

		base_stripe_t get_base_stripe_colours(
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) const;
	};
}
