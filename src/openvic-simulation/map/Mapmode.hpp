#pragma once

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

#include <function2/function2.hpp>

namespace OpenVic {
	struct MapmodeManager;
	struct MapInstance;
	struct ProvinceInstance;
	struct CountryInstance;

	struct Mapmode : HasIdentifier, HasIndex<int32_t> {
		friend struct MapmodeManager;

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
		std::string PROPERTY(localisation_key);
		const bool PROPERTY_CUSTOM_PREFIX(parchment_mapmode_allowed, is);

		Mapmode(
			std::string_view new_identifier,
			index_t new_index,
			colour_func_t new_colour_func,
			std::string_view new_localisation_key = {},
			bool new_parchment_mapmode_allowed = true
		);

	public:
		static const Mapmode ERROR_MAPMODE;

		Mapmode(Mapmode&&) = default;

		base_stripe_t get_base_stripe_colours(
			MapInstance const& map_instance, ProvinceInstance const& province,
			CountryInstance const* player_country, ProvinceInstance const* selected_province
		) const;
	};

	struct MapmodeManager {
	private:
		IdentifierRegistry<Mapmode> IDENTIFIER_REGISTRY(mapmode);

	public:
		MapmodeManager() = default;

		bool add_mapmode(
			std::string_view identifier,
			Mapmode::colour_func_t colour_func,
			std::string_view localisation_key = {},
			bool parchment_mapmode_allowed = true
		);

		/* The mapmode colour image contains of a list of base colours and stripe colours. Each colour is four bytes
		 * in RGBA format, with the alpha value being used to interpolate with the terrain colour, so A = 0 is fully terrain
		 * and A = 255 is fully the RGB colour packaged with A. The base and stripe colours for each province are packed
		 * together adjacently, so each province's entry is 8 bytes long. The list contains ProvinceDefinition::MAX_INDEX + 1
		 * entries, that is the maximum allowed number of provinces plus one for the index-zero "null province". */
		bool generate_mapmode_colours(
			MapInstance const& map_instance, Mapmode const* mapmode,
			CountryInstance const* player_country, ProvinceInstance const* selected_province,
			uint8_t* target
		) const;

		bool setup_mapmodes();
	};
}
