#include "TerrainType.hpp"

#include <limits>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TerrainType::TerrainType(
	std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_modifier, bool new_is_water
) : HasIdentifierAndColour { new_identifier, new_colour, false, false }, modifier { std::move(new_modifier) },
	is_water { new_is_water } {}

ModifierValue const& TerrainType::get_modifier() const {
	return modifier;
}

bool TerrainType::get_is_water() const {
	return is_water;
}

TerrainTypeMapping::TerrainTypeMapping(
	std::string_view new_identifier, TerrainType const& new_type, std::vector<index_t>&& new_terrain_indicies,
	index_t new_priority, bool new_has_texture
) : HasIdentifier { new_identifier }, type { new_type }, terrain_indicies { std::move(new_terrain_indicies) },
	priority { new_priority }, has_texture { new_has_texture } {}

TerrainType const& TerrainTypeMapping::get_type() const {
	return type;
}

std::vector<TerrainTypeMapping::index_t> const& TerrainTypeMapping::get_terrain_indicies() const {
	return terrain_indicies;
}

TerrainTypeMapping::index_t TerrainTypeMapping::get_priority() const {
	return priority;
}

bool TerrainTypeMapping::get_has_texture() const {
	return has_texture;
}

TerrainTypeManager::TerrainTypeManager()
	: terrain_types { "terrain types" }, terrain_type_mappings { "terrain type mappings" } {}

bool TerrainTypeManager::add_terrain_type(
	std::string_view identifier, colour_t colour, ModifierValue&& values, bool is_water
) {
	if (identifier.empty()) {
		Logger::error("Invalid terrain type identifier - empty!");
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid terrain type colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	return terrain_types.add_item({ identifier, colour, std::move(values), is_water });
}

bool TerrainTypeManager::add_terrain_type_mapping(
	std::string_view identifier, TerrainType const* type, std::vector<TerrainTypeMapping::index_t>&& terrain_indicies,
	TerrainTypeMapping::index_t priority, bool has_texture
) {
	if (!terrain_types_are_locked()) {
		Logger::error("Cannot register terrain type mappings until terrain types are locked!");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid terrain type mapping identifier - empty!");
		return false;
	}
	if (type == nullptr) {
		Logger::error("Null terrain type for mapping ", identifier);
		return false;
	}
	bool ret = true;
	for (TerrainTypeMapping::index_t idx : terrain_indicies) {
		const terrain_type_mappings_map_t::const_iterator it = terrain_type_mappings_map.find(idx);
		if (it == terrain_type_mappings_map.end()) {
			terrain_type_mappings_map.emplace(idx, terrain_type_mappings.size());
		} else {
			Logger::error(
				"Terrain index ", static_cast<size_t>(idx), " cannot map to ", identifier, " as it already maps to ",
				terrain_type_mappings.get_item_by_index(it->second)
			);
			ret = false;
		}
	}
	ret &= terrain_type_mappings.add_item({ identifier, *type, std::move(terrain_indicies), priority, has_texture });
	return ret;
}

node_callback_t TerrainTypeManager::_load_terrain_type_categories(ModifierManager const& modifier_manager) {
	return [this, &modifier_manager](ast::NodeCPtr root) -> bool {
		const bool ret = expect_dictionary_reserve_length(terrain_types,
			[this, &modifier_manager](std::string_view type_key, ast::NodeCPtr type_node) -> bool {
				ModifierValue values;
				colour_t colour = NULL_COLOUR;
				bool is_water = false;
				bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(values),
					"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
					"is_water", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_water))
				)(type_node);
				ret &= add_terrain_type(type_key, colour, std::move(values), is_water);
				return ret;
			}
		)(root);
		lock_terrain_types();
		return ret;
	};
}

bool TerrainTypeManager::_load_terrain_type_mapping(std::string_view mapping_key, ast::NodeCPtr mapping_value) {
	if (terrain_texture_limit <= 0) {
		Logger::error("Cannot define terrain type mapping before terrain texture limit: ", mapping_key);
		return false;
	}
	if (!terrain_types_are_locked()) {
		Logger::error("Cannot define terrain type mapping before categories: ", mapping_key);
		return false;
	}
	TerrainType const* type = nullptr;
	std::vector<TerrainTypeMapping::index_t> terrain_indicies;
	TerrainTypeMapping::index_t priority = 0;
	bool has_texture = true;

	bool ret = expect_dictionary_keys(
		"type", ONE_EXACTLY, expect_terrain_type_identifier(assign_variable_callback_pointer(type)),
		"color", ONE_EXACTLY, expect_list_reserve_length(terrain_indicies, expect_uint<TerrainTypeMapping::index_t>(
			[&terrain_indicies](TerrainTypeMapping::index_t val) -> bool {
				if (std::find(terrain_indicies.begin(), terrain_indicies.end(), val) == terrain_indicies.end()) {
					terrain_indicies.push_back(val);
					return true;
				}
				Logger::error("Repeat terrain type mapping index: ", val);
				return false;
			}
		)),
		"priority", ZERO_OR_ONE, expect_uint(assign_variable_callback(priority)),
		"has_texture", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_texture))
	)(mapping_value);
	if (has_texture && ++terrain_texture_count == terrain_texture_limit + 1) {
		Logger::warning("More terrain textures than limit!");
	}
	ret &= add_terrain_type_mapping(mapping_key, type, std::move(terrain_indicies), priority, has_texture);
	return true;
}

TerrainTypeMapping const* TerrainTypeManager::get_terrain_type_mapping_for(TerrainTypeMapping::index_t idx) const {
	const terrain_type_mappings_map_t::const_iterator it = terrain_type_mappings_map.find(idx);
	if (it != terrain_type_mappings_map.end()) {
		return terrain_type_mappings.get_item_by_index(it->second);
	}
	return nullptr;
}

TerrainTypeMapping::index_t TerrainTypeManager::get_terrain_texture_limit() const {
	return terrain_texture_limit;
}

bool TerrainTypeManager::load_terrain_types(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const bool ret = expect_dictionary_keys_and_length_and_default(
		[this](size_t size) -> size_t {
			terrain_type_mappings.reserve(size - 2);
			return size;
		},
		std::bind(&TerrainTypeManager::_load_terrain_type_mapping, this, std::placeholders::_1, std::placeholders::_2),
		"terrain", ONE_EXACTLY, expect_uint(assign_variable_callback(terrain_texture_limit)),
		"categories", ONE_EXACTLY, _load_terrain_type_categories(modifier_manager)
	)(root);
	lock_terrain_type_mappings();
	return ret;
}
