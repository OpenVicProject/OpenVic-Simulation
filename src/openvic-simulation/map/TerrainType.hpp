#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	// Using HasColour rather than HasIdentifierAndColour to avoid needing virtual inheritance
	// (extending Modifier is more useful than extending HasIdentifierAndColour).
	struct TerrainType : HasIndex<TerrainType>, Modifier, HasColour {
	private:
		ModifierValue PROPERTY(modifier);
		fixed_point_t PROPERTY(movement_cost);
		fixed_point_t PROPERTY(defence_bonus);
		fixed_point_t PROPERTY(combat_width_percentage_change);
		const bool PROPERTY(is_water);

	public:
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
		TerrainType const& PROPERTY(type);
		memory::vector<index_t> PROPERTY(terrain_indices);
		const index_t PROPERTY(priority);
		const bool PROPERTY(has_texture);

	public:
		TerrainTypeMapping(
			std::string_view new_identifier,
			TerrainType const& new_type,
			memory::vector<index_t>&& new_terrain_indices,
			index_t new_priority,
			bool new_has_texture
		);
		TerrainTypeMapping(TerrainTypeMapping&&) = default;
	};

	struct TerrainTypeManager {
	private:
		using terrain_type_mappings_map_t = ordered_map<TerrainTypeMapping::index_t, size_t>;

		IdentifierRegistry<TerrainType> IDENTIFIER_REGISTRY(terrain_type);
		IdentifierRegistry<TerrainTypeMapping> IDENTIFIER_REGISTRY(terrain_type_mapping);
		terrain_type_mappings_map_t terrain_type_mappings_map;

		TerrainTypeMapping::index_t terrain_texture_limit = 0, terrain_texture_count = 0;

		NodeTools::node_callback_t _load_terrain_type_categories(ModifierManager const& modifier_manager);
		bool _load_terrain_type_mapping(std::string_view key, ast::NodeCPtr value);

	public:
		bool add_terrain_type(
			std::string_view identifier,
			colour_t colour,
			ModifierValue&& values,
			fixed_point_t movement_cost,
			fixed_point_t defence_bonus,
			fixed_point_t combat_width_percentage_change,
			bool is_water
		);

		bool add_terrain_type_mapping(
			std::string_view identifier,
			TerrainType const* type,
			memory::vector<TerrainTypeMapping::index_t>&& terrain_indices,
			TerrainTypeMapping::index_t priority,
			bool has_texture
		);

		TerrainTypeMapping const* get_terrain_type_mapping_for(TerrainTypeMapping::index_t idx) const;

		TerrainTypeMapping::index_t get_terrain_texture_limit() const;

		bool load_terrain_types(ModifierManager const& modifier_manager, ast::NodeCPtr root);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
