#pragma once

#include "openvic-simulation/Modifier.hpp"

namespace OpenVic {
	struct TerrainTypeManager;

	struct TerrainType : HasIdentifierAndColour, ModifierValue {
		friend struct TerrainTypeManager;

	private:
		const ModifierValue modifier;
		const bool is_water;

		TerrainType(std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_modifier, bool new_is_water);

	public:
		TerrainType(TerrainType&&) = default;

		ModifierValue const& get_modifier() const;
		bool get_is_water() const;
	};

	struct TerrainTypeMapping : HasIdentifier {
		friend struct TerrainTypeManager;

		using index_t = uint8_t;

	private:
		TerrainType const& type;
		const std::vector<index_t> terrain_indicies;
		const index_t priority;
		const bool has_texture;

		TerrainTypeMapping(std::string_view new_identifier, TerrainType const& new_type, std::vector<index_t>&& new_terrain_indicies, index_t new_priority, bool new_has_texture);

	public:
		TerrainTypeMapping(TerrainTypeMapping&&) = default;

		TerrainType const& get_type() const;
		std::vector<index_t> const& get_terrain_indicies() const;
		index_t get_priority() const;
		bool get_has_texture() const;
	};

	struct TerrainTypeManager {
	private:
		using terrain_type_mappings_map_t = std::map<TerrainTypeMapping::index_t, size_t>;
		IdentifierRegistry<TerrainType> terrain_types;
		IdentifierRegistry<TerrainTypeMapping> terrain_type_mappings;
		terrain_type_mappings_map_t terrain_type_mappings_map;

		TerrainTypeMapping::index_t terrain_texture_limit = 0, terrain_texture_count = 0;

		NodeTools::node_callback_t _load_terrain_type_categories(ModifierManager const& modifier_manager);
		bool _load_terrain_type_mapping(std::string_view key, ast::NodeCPtr value);

	public:
		TerrainTypeManager();

		bool add_terrain_type(std::string_view identifier, colour_t colour, ModifierValue&& values, bool is_water);
		IDENTIFIER_REGISTRY_ACCESSORS(terrain_type)

		bool add_terrain_type_mapping(std::string_view identifier, TerrainType const* type,
			std::vector<TerrainTypeMapping::index_t>&& terrain_indicies, TerrainTypeMapping::index_t priority, bool has_texture);
		IDENTIFIER_REGISTRY_ACCESSORS(terrain_type_mapping)

		TerrainTypeMapping const* get_terrain_type_mapping_for(TerrainTypeMapping::index_t idx) const;

		TerrainTypeMapping::index_t get_terrain_texture_limit() const;

		bool load_terrain_types(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
}
