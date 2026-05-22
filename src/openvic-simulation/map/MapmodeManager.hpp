#pragma once

#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct MapDefinition;
	struct MapInstance;
	struct ProvinceInstance;
	struct CountryInstance;

	struct MapmodeManager {
	private:
		IdentifierRegistry<Mapmode> IDENTIFIER_REGISTRY(mapmode);

	public:
		constexpr MapmodeManager() {};

		bool add_mapmode(
			std::string_view identifier,
			Mapmode::colour_func_t colour_func,
			std::string_view localisation_key = {},
			bool parchment_mapmode_allowed = true
		);

		/* The mapmode colour image contains of a list of base colours and stripe colours. Each colour is four bytes
		 * in RGBA format, with the alpha value being used to interpolate with the terrain colour, so A = 0 is fully terrain
		 * and A = 255 is fully the RGB colour packaged with A. The base and stripe colours for each province are packed
		 * together adjacently, so each province's entry is 8 bytes long.
		 * The list contains all provinces indexed by their number + index 0 for the "null province". */
		bool generate_mapmode_colours(
			MapInstance const& map_instance, Mapmode const* mapmode,
			CountryInstance const* player_country, ProvinceInstance const* selected_province,
			uint8_t* target
		) const;

		bool setup_mapmodes(MapDefinition const& map_definition, BuildingTypeManager const& building_type_manager);
	};
}
