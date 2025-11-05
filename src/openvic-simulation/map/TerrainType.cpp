#include "TerrainType.hpp"

#include <cstddef>
#include <string_view>
#include <utility>

#include "openvic-simulation/core/FormatValidate.hpp"
#include "openvic-simulation/core/container/HasIdentifier.hpp"
#include "openvic-simulation/core/container/IndexedFlatMap.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/object/Colour.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TerrainType::TerrainType(
	index_t new_index,
	std::string_view new_identifier,
	colour_t new_colour,
	ModifierValue&& new_modifier,
	fixed_point_t new_movement_cost,
	fixed_point_t new_defence_bonus,
	fixed_point_t new_combat_width_percentage_change,
	bool new_is_water
) : HasIndex<TerrainType> { new_index },
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

bool TerrainTypeManager::generate_modifiers(ModifierManager& modifier_manager) const {
	using enum ModifierEffect::format_t;
	IndexedFlatMap<TerrainType, ModifierEffectCache::unit_terrain_effects_t>& unit_terrain_effects =
		modifier_manager.modifier_effect_cache.unit_terrain_effects;

	unit_terrain_effects = std::move(decltype(ModifierEffectCache::unit_terrain_effects){get_terrain_types()});

	bool ret = true;
	for (TerrainType const& terrain_type : get_terrain_types()) {
		const std::string_view identifier = terrain_type.get_identifier();
		ModifierEffectCache::unit_terrain_effects_t& this_unit_terrain_effects = unit_terrain_effects.at(terrain_type);
		ret &= modifier_manager.register_unit_terrain_modifier_effect(
			this_unit_terrain_effects.attack, ModifierManager::get_flat_identifier("attack", identifier),
			FORMAT_x100_1DP_PC_POS, "UA_ATTACK"
		);
		ret &= modifier_manager.register_unit_terrain_modifier_effect(
			this_unit_terrain_effects.defence, ModifierManager::get_flat_identifier("defence", identifier),
			FORMAT_x100_1DP_PC_POS, "UA_DEFENCE"
		);
		ret &= modifier_manager.register_unit_terrain_modifier_effect(
			this_unit_terrain_effects.movement, ModifierManager::get_flat_identifier("movement", identifier),
			FORMAT_x100_1DP_PC_POS, "UA_MOVEMENT"
		);
		ret &= modifier_manager.register_unit_terrain_modifier_effect(
			this_unit_terrain_effects.attrition, ModifierManager::get_flat_identifier("attrition", identifier),
			FORMAT_x1_1DP_PC_NEG, "UA_ATTRITION"
		);
	}
	return ret;
}

bool TerrainTypeManager::add_terrain_type(
	std::string_view identifier,
	colour_t colour,
	ModifierValue&& values,
	fixed_point_t movement_cost,
	fixed_point_t defence_bonus,
	fixed_point_t combat_width_percentage_change,
	bool is_water
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid terrain type identifier - empty!");
		return false;
	}

	if (movement_cost < 0) {
		spdlog::error_s(
			"Invalid movement cost for terrain type \"{}\": {} (cannot be negative!)",
			identifier, movement_cost
		);
		return false;
	}

	return terrain_types.emplace_item(
		identifier,
		get_terrain_type_count(), identifier,
		colour, std::move(values), movement_cost, defence_bonus, combat_width_percentage_change, is_water
	);
}

bool TerrainTypeManager::add_terrain_type_mapping(
	std::string_view identifier,
	TerrainType const* type,
	memory::vector<TerrainTypeMapping::index_t>&& terrain_indices,
	TerrainTypeMapping::index_t priority,
	bool has_texture
) {
	if (!terrain_types_are_locked()) {
		spdlog::error_s("Cannot register terrain type mappings until terrain types are locked!");
		return false;
	}

	if (identifier.empty()) {
		spdlog::error_s("Invalid terrain type mapping identifier - empty!");
		return false;
	}

	if (type == nullptr) {
		spdlog::error_s("Null terrain type for mapping {}", identifier);
		return false;
	}

	bool ret = true;

	for (TerrainTypeMapping::index_t idx : terrain_indices) {
		const terrain_type_mappings_map_t::const_iterator it = terrain_type_mappings_map.find(idx);

		if (it == terrain_type_mappings_map.end()) {
			terrain_type_mappings_map.emplace(idx, terrain_type_mappings.size());
		} else {
			spdlog::error_s(
				"Terrain index {} cannot map to {} as it already maps to {}",
				static_cast<size_t>(idx), identifier, ovfmt::validate(terrain_type_mappings.get_item_by_index(it->second))
			);
			ret = false;
		}
	}

	ret &= terrain_type_mappings.emplace_item(
		identifier,
		identifier, *type, std::move(terrain_indices), priority, has_texture
	);

	return ret;
}

node_callback_t TerrainTypeManager::_load_terrain_type_categories(ModifierManager const& modifier_manager) {
	return [this, &modifier_manager](ast::NodeCPtr root) -> bool {
		const bool ret = expect_dictionary_reserve_length(terrain_types,
			[this, &modifier_manager](std::string_view type_key, ast::NodeCPtr type_node) -> bool {
				ModifierValue values;
				fixed_point_t movement_cost, defence_bonus, combat_width_percentage_change;
				colour_t colour = colour_t::null();
				bool is_water = false;

				bool ret = NodeTools::expect_dictionary_keys_and_default(
					modifier_manager.expect_terrain_modifier(values),
					"movement_cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(movement_cost)),
					"defence", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(defence_bonus)),
					"combat_width", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(combat_width_percentage_change)),
					"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
					"is_water", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_water))
				)(type_node);

				ret &= add_terrain_type(
					type_key, colour, std::move(values), movement_cost, defence_bonus, combat_width_percentage_change, is_water
				);

				return ret;
			}
		)(root);

		lock_terrain_types();

		return ret;
	};
}

bool TerrainTypeManager::_load_terrain_type_mapping(std::string_view mapping_key, ast::NodeCPtr mapping_value) {
	if (terrain_texture_limit <= 0) {
		spdlog::error_s("Cannot define terrain type mapping before terrain texture limit: {}", mapping_key);
		return false;
	}

	if (!terrain_types_are_locked()) {
		spdlog::error_s("Cannot define terrain type mapping before categories: {}", mapping_key);
		return false;
	}

	TerrainType const* type = nullptr;
	memory::vector<TerrainTypeMapping::index_t> terrain_indices;
	TerrainTypeMapping::index_t priority = 0;
	bool has_texture = true;

	bool ret = expect_dictionary_keys_and_default(
		[mapping_key](std::string_view key, ast::NodeCPtr value) -> bool {
			spdlog::warn_s("Invalid key {} in terrain mapping {}", key, mapping_key);
			return true;
		},
		"type", ONE_EXACTLY, expect_terrain_type_identifier(assign_variable_callback_pointer(type)),
		"color", ONE_EXACTLY, expect_list_reserve_length(terrain_indices, expect_uint<TerrainTypeMapping::index_t>(
			[&terrain_indices](TerrainTypeMapping::index_t val) -> bool {
				if (std::find(terrain_indices.begin(), terrain_indices.end(), val) == terrain_indices.end()) {
					terrain_indices.push_back(val);
					return true;
				}
				spdlog::error_s("Repeat terrain type mapping index: {}", val);
				return false;
			}
		)),
		"priority", ZERO_OR_ONE, expect_uint(assign_variable_callback(priority)),
		"has_texture", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_texture))
	)(mapping_value);

	if (has_texture && ++terrain_texture_count == terrain_texture_limit + 1) {
		spdlog::warn_s("More terrain textures than limit!");
	}

	ret &= add_terrain_type_mapping(mapping_key, type, std::move(terrain_indices), priority, has_texture);

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
	const bool ret = expect_dictionary_keys_reserve_length_and_default(
		terrain_type_mappings,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			return _load_terrain_type_mapping(key, value);
		},
		"terrain", ONE_EXACTLY, expect_uint(assign_variable_callback(terrain_texture_limit)),
		"categories", ONE_EXACTLY, _load_terrain_type_categories(modifier_manager)
	)(root);

	lock_terrain_type_mappings();

	return ret;
}
