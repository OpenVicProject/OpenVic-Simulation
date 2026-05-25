#pragma once

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct TerrainTypeManager {
	private:
		using terrain_type_mappings_map_t = ordered_map<TerrainTypeMapping::index_t, size_t>;

		IdentifierRegistry<TerrainType> IDENTIFIER_REGISTRY(terrain_type);
		IdentifierRegistry<TerrainTypeMapping> IDENTIFIER_REGISTRY(terrain_type_mapping);
		terrain_type_mappings_map_t terrain_type_mappings_map;

		TerrainTypeMapping::index_t terrain_texture_limit = 0, terrain_texture_count = 0;

		NodeTools::node_callback_t _load_terrain_type_categories(ModifierManager const& modifier_manager);
		bool _load_terrain_type_mapping(std::string_view key, ovdl::v2script::ast::Node const* value);

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

		bool load_terrain_types(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
