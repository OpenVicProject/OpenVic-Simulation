#pragma once

#include <cstdint>
#include <string_view>

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {
	struct TerrainType;
	struct TerrainTypeManager;
	struct Culture;
	struct CountryInstance;

	struct UnitType : HasIdentifier {
		using icon_t = uint32_t;
		using terrain_modifiers_t = ordered_map<TerrainType const*, Modifier>;

		enum struct unit_category_t : uint8_t {
			INVALID_UNIT_CATEGORY, INFANTRY, CAVALRY, SUPPORT, SPECIAL, BIG_SHIP, LIGHT_SHIP, TRANSPORT
		};

		struct unit_type_args_t {
			using terrain_modifier_values_t = ordered_map<TerrainType const*, ModifierValue>;

			icon_t icon = 0;
			unit_category_t unit_category = unit_category_t::INVALID_UNIT_CATEGORY;
			// TODO defaults for move_sound and select_sound
			std::string_view sprite, move_sound, select_sound;
			bool starts_unlocked = true, floating_flag = false;
			uint32_t priority = 0;
			fixed_point_t max_strength = 0, default_organisation = 0, maximum_speed = 0, weighted_value = 0,
				supply_consumption = 0;
			Timespan build_time;
			fixed_point_map_t<GoodDefinition const*> build_cost, supply_cost;
			terrain_modifier_values_t terrain_modifier_values;

			unit_type_args_t() {};
			unit_type_args_t(unit_type_args_t&&) = default;
		};

	private:
		memory::string PROPERTY(sprite);
		memory::string PROPERTY(move_sound);
		memory::string PROPERTY(select_sound);

		fixed_point_map_t<GoodDefinition const*> PROPERTY(build_cost);
		fixed_point_map_t<GoodDefinition const*> PROPERTY(supply_cost);

		terrain_modifiers_t PROPERTY(terrain_modifiers);

	protected:
		/* Non-const reference unit_args so variables can be moved from it. */
		UnitType(std::string_view new_identifier, unit_branch_t new_branch, unit_type_args_t& unit_args);

	public:
		const unit_branch_t branch; /* type in defines */
		const unit_category_t unit_category;
		const bool starts_unlocked;
		const bool has_floating_flag;
		const icon_t icon;
		const uint32_t priority;
		const fixed_point_t max_strength;
		const fixed_point_t default_organisation;
		const fixed_point_t maximum_speed;
		const fixed_point_t weighted_value;
		const Timespan build_time;
		const fixed_point_t supply_consumption;

		UnitType(UnitType&&) = default;
	};

	template<>
	struct UnitTypeBranched<unit_branch_t::LAND> : UnitType, HasIndex<RegimentType, regiment_type_index_t> {
		static constexpr regiment_allowed_cultures_t allowed_cultures_get_most_permissive(
			regiment_allowed_cultures_t lhs, regiment_allowed_cultures_t rhs
		) {
			return std::min(lhs, rhs);
		}

		struct regiment_type_args_t {
			regiment_allowed_cultures_t allowed_cultures = regiment_allowed_cultures_t::ALL_CULTURES;
			std::string_view sprite_override, sprite_mount, sprite_mount_attach_node;
			// TODO - represent these as modifier effects, so that they can be combined with tech, inventions,
			// leader bonuses, etc. and applied to unit instances all in one go (same for ShipTypes below)
			fixed_point_t reconnaissance = 0, attack = 0, defence = 0, discipline = 0, support = 0, maneuver = 0,
				siege = 0;

			constexpr regiment_type_args_t() {};
			regiment_type_args_t(regiment_type_args_t&&) = default;
		};

	private:
		memory::string PROPERTY(sprite_override);
		memory::string PROPERTY(sprite_mount);
		memory::string PROPERTY(sprite_mount_attach_node);

	public:
		const regiment_allowed_cultures_t allowed_cultures;
		const fixed_point_t reconnaissance;
		const fixed_point_t attack;
		const fixed_point_t defence;
		const fixed_point_t discipline;
		const fixed_point_t support;
		const fixed_point_t maneuver;
		const fixed_point_t siege;

		UnitTypeBranched(
			index_t new_index, std::string_view new_identifier,
			unit_type_args_t& unit_args, regiment_type_args_t const& regiment_type_args
		);
		UnitTypeBranched(UnitTypeBranched&&) = default;
	};

	template<>
	struct UnitTypeBranched<unit_branch_t::NAVAL> : UnitType, HasIndex<ShipType, ship_type_index_t> {
		struct ship_type_args_t {
			icon_t naval_icon = 0;
			bool sail = false, transport = false, capital = false, build_overseas = false;
			uint32_t min_port_level = 0;
			int32_t limit_per_port = 0;
			fixed_point_t colonial_points = 0, supply_consumption_score = 0, hull = 0, gun_power = 0, fire_range = 0,
				evasion = 0, torpedo_attack = 0;

			constexpr ship_type_args_t() {};
			ship_type_args_t(ship_type_args_t&&) = default;
		};

	public:
		const bool can_sail;
		const bool is_transport;
		const bool is_capital;
		const bool can_build_overseas;

		const icon_t naval_icon;
		const fixed_point_t colonial_points;
		const uint32_t min_port_level;
		const int32_t limit_per_port;
		const fixed_point_t supply_consumption_score;

		const fixed_point_t hull;
		const fixed_point_t gun_power;
		const fixed_point_t fire_range;
		const fixed_point_t evasion;
		const fixed_point_t torpedo_attack;

		UnitTypeBranched(
			index_t new_index, std::string_view new_identifier,
			unit_type_args_t& unit_args, ship_type_args_t const& ship_type_args
		);
		UnitTypeBranched(UnitTypeBranched&&) = default;
	};

	struct UnitTypeManager {
	private:
		IdentifierPointerRegistry<UnitType> IDENTIFIER_REGISTRY(unit_type);
		IdentifierRegistry<RegimentType> IDENTIFIER_REGISTRY(regiment_type);
		IdentifierRegistry<ShipType> IDENTIFIER_REGISTRY(ship_type);

	public:
		void reserve_all_unit_types(size_t size);
		void lock_all_unit_types();

		bool add_regiment_type(
			std::string_view identifier, UnitType::unit_type_args_t& unit_args,
			RegimentType::regiment_type_args_t const& regiment_type_args
		);
		bool add_ship_type(
			std::string_view identifier, UnitType::unit_type_args_t& unit_args,
			ShipType::ship_type_args_t const& ship_type_args
		);

		static NodeTools::Callback<std::string_view> auto expect_branch_str(
			NodeTools::Callback<unit_branch_t> auto callback
		) {
			using enum unit_branch_t;
			static const string_map_t<unit_branch_t> branch_map {
				{ "land", LAND }, { "naval", NAVAL }, { "sea", NAVAL }
			};
			return NodeTools::expect_mapped_string(branch_map, callback);
		}
		static NodeTools::NodeCallback auto expect_branch_identifier(NodeTools::Callback<unit_branch_t> auto callback) {
			return NodeTools::expect_identifier(expect_branch_str(callback));
		}

		bool load_unit_type_file(
			GoodDefinitionManager const& good_definition_manager, TerrainTypeManager const& terrain_type_manager,
			ModifierManager const& modifier_manager, ovdl::v2script::Parser const& parser
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
