#include "TerrainType.hpp"

#include <limits>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TerrainType::TerrainType(const std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_values, bool new_is_water)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false }, ModifierValue { std::move(new_values) }, is_water { new_is_water } {}

bool TerrainType::get_is_water() const {
	return is_water;
}

TerrainTypeMapping::TerrainTypeMapping(const std::string_view new_identifier, TerrainType const& new_type,
	std::vector<index_t>&& new_terrain_indicies, index_t new_priority, bool new_has_texture)
	: HasIdentifier { new_identifier }, type { new_type }, terrain_indicies { std::move(new_terrain_indicies) },
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

TerrainTypeManager::TerrainTypeManager() : terrain_types { "terrain types" }, terrain_type_mappings { "terrain type mappings" } {}

bool TerrainTypeManager::add_terrain_type(const std::string_view identifier, colour_t colour, ModifierValue&& values, bool is_water) {
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

bool TerrainTypeManager::add_terrain_type_mapping(const std::string_view identifier, TerrainType const* type,
	std::vector<TerrainTypeMapping::index_t>&& terrain_indicies, TerrainTypeMapping::index_t priority, bool has_texture) {
	if (!terrain_types.is_locked()) {
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
			Logger::error("Terrain index ", static_cast<size_t>(idx), " cannot map to ", identifier,
				" as it already maps to ", terrain_type_mappings.get_item_by_index(it->second));
			ret = false;
		}
	}
	ret &= terrain_type_mappings.add_item({ identifier, *type, std::move(terrain_indicies), priority, has_texture });
	return ret;
}

bool TerrainTypeManager::_load_terrain_type_categories(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(terrain_types,
		[this, &modifier_manager](std::string_view type_key, ast::NodeCPtr type_node) -> bool {
			ModifierValue values;
			colour_t colour = NULL_COLOUR;
			bool is_water = false;
			bool has_colour = false, has_is_water = false;
			bool ret = modifier_manager.expect_modifier_value(move_variable_callback(values),
				[&colour, &has_colour, &is_water, &has_is_water](std::string_view key, ast::NodeCPtr value) -> bool {
					if (key == "color") {
						if (!has_colour) {
							has_colour = true;
							return expect_colour(assign_variable_callback(colour))(value);
						} else {
							Logger::error("Duplicate terrain type colour key!");
							return false;
						}
					} else if (key == "is_water") {
						if (!has_is_water) {
							has_is_water = true;
							return expect_bool(assign_variable_callback(is_water))(value);
						} else {
							Logger::error("Duplicate terrain type is_water key!");
							return false;
						}
					} else {
						Logger::error("Invalid terrain type entry key: ", key);
						return false;
					}
				} 
			)(type_node);
			if (!has_colour) {
				Logger::error("Terrain type missing color key: ", type_key);
				ret = false;
			}
			ret &= add_terrain_type(type_key, colour, std::move(values), is_water);
			return ret;
		}
	)(root);
	terrain_types.lock();
	return ret;
}

bool TerrainTypeManager::_load_terrain_type_mapping(std::string_view mapping_key, ast::NodeCPtr mapping_value) {
	TerrainType const* type = nullptr;
	std::vector<TerrainTypeMapping::index_t> terrain_indicies;
	TerrainTypeMapping::index_t priority = 0;
	bool has_texture = true;

	bool ret = expect_dictionary_keys(
		"type", ONE_EXACTLY, expect_terrain_type_identifier(assign_variable_callback_pointer(type)),
		"color", ONE_EXACTLY, expect_list_reserve_length(terrain_indicies, expect_uint(
			[&terrain_indicies](uint64_t val) -> bool {
				if (val <= std::numeric_limits<TerrainTypeMapping::index_t>::max()) {
					TerrainTypeMapping::index_t index = val;
					if (std::find(terrain_indicies.begin(), terrain_indicies.end(), index) == terrain_indicies.end()) {
						terrain_indicies.push_back(val);
						return true;
					}
					Logger::error("Repeat terrain type mapping index: ", val);
					return false;
				}
				Logger::error("Index too big for terrain type mapping index: ", val);
				return false;
			}
		)),
		"priority", ZERO_OR_ONE, expect_uint(assign_variable_callback_uint("terrain type mapping priority", priority)),
		"has_texture", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_texture))
	)(mapping_value);
	if (has_texture) {
		if (++terrain_texture_count == terrain_texture_limit + 1) {
			Logger::error("More terrain textures than limit!");
			ret = false;
		}
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
	bool terrain = false, categories = false;
	bool ret = expect_dictionary_and_length(
		[this](size_t size) -> size_t {
			terrain_type_mappings.reserve(size - 2);
			return size;
		},
		[this, &terrain, &categories, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "terrain") {
				if (!terrain) {
					terrain = true;
					return expect_uint(assign_variable_callback_uint("terrain texture limit", terrain_texture_limit))(value);
				} else {
					Logger::error("Duplicate terrain key!");
					return false;
				}
			} else if (key == "categories") {
				if (!categories) {
					categories = true;
					return _load_terrain_type_categories(modifier_manager, value);
				} else {
					Logger::error("Duplicate categories key!");
					return false;
				}
			} else if (terrain && categories) {
				return _load_terrain_type_mapping(key, value);
			} else {
				Logger::error("Cannot define terrain type mapping before terrain and categories keys: ", key);
				return false;
			}
		}
	)(root);
	if (!terrain) {
		Logger::error("Missing expected key: \"terrain\"");
		ret = false;
	}
	if (!categories) {
		Logger::error("Missing expected key: \"categories\"");
		ret = false;
	}
	terrain_type_mappings.lock();
	return ret;
}
