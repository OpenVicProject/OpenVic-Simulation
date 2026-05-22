#pragma once

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	// Using HasColour rather than HasIdentifierAndColour to avoid needing virtual inheritance
	// (extending Modifier is more useful than extending HasIdentifierAndColour).
	struct TerrainType : HasIndex<TerrainType, terrain_type_index_t>, Modifier, public HasColour {
	private:
		ModifierValue PROPERTY(modifier);
		fixed_point_t PROPERTY(movement_cost);
		fixed_point_t PROPERTY(defence_bonus);
		fixed_point_t PROPERTY(combat_width_percentage_change);

	public:
		const bool is_water;

		TerrainType(
			index_t new_index,
			std::string_view new_identifier,
			colour_t new_colour,
			ModifierValue&& new_modifier,
			fixed_point_t new_movement_cost,
			fixed_point_t new_defence_bonus,
			fixed_point_t new_combat_width_percentage_change,
			bool new_is_water
		);
		TerrainType(TerrainType&&) = default;
	};

	struct TerrainTypeMapping : HasIdentifier {
		using index_t = uint8_t;

	private:
		memory::vector<index_t> SPAN_PROPERTY(terrain_indices);

	public:
		TerrainType const& type;
		const index_t priority;
		const bool has_texture;

		TerrainTypeMapping(
			std::string_view new_identifier,
			TerrainType const& new_type,
			memory::vector<index_t>&& new_terrain_indices,
			index_t new_priority,
			bool new_has_texture
		);
		TerrainTypeMapping(TerrainTypeMapping&&) = default;
	};
}
