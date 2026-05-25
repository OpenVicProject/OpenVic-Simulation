#include "TerrainType.hpp"

#include <string_view>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

using namespace OpenVic;

TerrainType::TerrainType(
	index_t new_index,
	std::string_view new_identifier,
	colour_t new_colour,
	ModifierValue&& new_modifier,
	fixed_point_t new_movement_cost,
	fixed_point_t new_defence_bonus,
	fixed_point_t new_combat_width_percentage_change,
	bool new_is_water
) : HasIndex { new_index },
	Modifier { new_identifier, std::move(new_modifier), modifier_type_t::TERRAIN },
	HasColour { new_colour, false },
	movement_cost { new_movement_cost },
	defence_bonus { new_defence_bonus },
	combat_width_percentage_change { new_combat_width_percentage_change },
	is_water { new_is_water } {}

TerrainTypeMapping::TerrainTypeMapping(
	std::string_view new_identifier,
	TerrainType const& new_type,
	memory::vector<index_t>&& new_terrain_indices,
	index_t new_priority,
	bool new_has_texture
) : HasIdentifier { new_identifier },
	type { new_type },
	terrain_indices { std::move(new_terrain_indices) },
	priority { new_priority },
	has_texture { new_has_texture } {}
