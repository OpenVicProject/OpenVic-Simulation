#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct TerrainType;
	struct TerrainTypeManager;

	struct UnitType : HasIdentifier {
		using icon_t = uint32_t;
		using terrain_modifiers_t = ordered_map<TerrainType const*, ModifierValue>;

		enum struct branch_t : uint8_t { INVALID_BRANCH, LAND, NAVAL };
		enum struct unit_category_t : uint8_t {
			INVALID_UNIT_CATEGORY, INFANTRY, CAVALRY, SUPPORT, SPECIAL, BIG_SHIP, LIGHT_SHIP, TRANSPORT
		};

		struct unit_type_args_t {
			icon_t icon = 0;
			unit_category_t unit_category = unit_category_t::INVALID_UNIT_CATEGORY;
			// TODO defaults for move_sound and select_sound
			std::string_view sprite, move_sound, select_sound;
			bool active = true, floating_flag = false;
			uint32_t priority = 0;
			fixed_point_t max_strength = 0, default_organisation = 0, maximum_speed = 0, weighted_value = 0,
				supply_consumption = 0;
			Timespan build_time;
			GoodDefinition::good_definition_map_t build_cost, supply_cost;
			terrain_modifiers_t terrain_modifiers;

			unit_type_args_t() = default;
			unit_type_args_t(unit_type_args_t&&) = default;
		};

	private:
		const branch_t PROPERTY(branch); /* type in defines */
		const icon_t PROPERTY(icon);
		std::string PROPERTY(sprite);
		const bool PROPERTY_CUSTOM_PREFIX(active, is);
		unit_category_t PROPERTY(unit_category);
		const bool PROPERTY_CUSTOM_PREFIX(floating_flag, has);

		const uint32_t PROPERTY(priority);
		const fixed_point_t PROPERTY(max_strength);
		const fixed_point_t PROPERTY(default_organisation);
		const fixed_point_t PROPERTY(maximum_speed);
		const fixed_point_t PROPERTY(weighted_value);

		std::string PROPERTY(move_sound);
		std::string PROPERTY(select_sound);

		const Timespan PROPERTY(build_time);
		GoodDefinition::good_definition_map_t PROPERTY(build_cost);
		const fixed_point_t PROPERTY(supply_consumption);
		GoodDefinition::good_definition_map_t PROPERTY(supply_cost);

		terrain_modifiers_t PROPERTY(terrain_modifiers);

	protected:
		/* Non-const reference unit_args so variables can be moved from it. */
		UnitType(std::string_view new_identifier, branch_t new_branch, unit_type_args_t& unit_args);

	public:
		UnitType(UnitType&&) = default;
	};

	template<UnitType::branch_t>
	struct UnitTypeBranched;

	template<>
	struct UnitTypeBranched<UnitType::branch_t::LAND> : UnitType {
		friend struct UnitTypeManager;

		enum struct allowed_cultures_t { ALL_CULTURES, ACCEPTED_CULTURES, PRIMARY_CULTURE };

		struct regiment_type_args_t {
			allowed_cultures_t allowed_cultures = allowed_cultures_t::ALL_CULTURES;
			std::string_view sprite_override, sprite_mount, sprite_mount_attach_node;
			// TODO - represent these as modifier effects, so that they can be combined with tech, inventions,
			// leader bonuses, etc. and applied to unit instances all in one go (same for ShipTypes below)
			fixed_point_t reconnaissance = 0, attack = 0, defence = 0, discipline = 0, support = 0, maneuver = 0,
				siege = 0;

			regiment_type_args_t() = default;
			regiment_type_args_t(regiment_type_args_t&&) = default;
		};

	private:
		const allowed_cultures_t PROPERTY(allowed_cultures);
		std::string PROPERTY(sprite_override);
		std::string PROPERTY(sprite_mount);
		std::string PROPERTY(sprite_mount_attach_node);
		const fixed_point_t PROPERTY(reconnaissance);
		const fixed_point_t PROPERTY(attack);
		const fixed_point_t PROPERTY(defence);
		const fixed_point_t PROPERTY(discipline);
		const fixed_point_t PROPERTY(support);
		const fixed_point_t PROPERTY(maneuver);
		const fixed_point_t PROPERTY(siege);

		UnitTypeBranched(
			std::string_view new_identifier, unit_type_args_t& unit_args, regiment_type_args_t const& regiment_type_args
		);

	public:
		UnitTypeBranched(UnitTypeBranched&&) = default;
	};

	using RegimentType = UnitTypeBranched<UnitType::branch_t::LAND>;

	template<>
	struct UnitTypeBranched<UnitType::branch_t::NAVAL> : UnitType {
		friend struct UnitTypeManager;

		struct ship_type_args_t {
			icon_t naval_icon = 0;
			bool sail = false, transport = false, capital = false, build_overseas = false;
			uint32_t min_port_level = 0;
			int32_t limit_per_port = 0;
			fixed_point_t colonial_points = 0, supply_consumption_score = 0, hull = 0, gun_power = 0, fire_range = 0,
				evasion = 0, torpedo_attack = 0;

			ship_type_args_t() = default;
			ship_type_args_t(ship_type_args_t&&) = default;
		};

	private:
		const icon_t PROPERTY(naval_icon);
		const bool PROPERTY_CUSTOM_PREFIX(sail, can);
		const bool PROPERTY_CUSTOM_PREFIX(transport, is);
		const bool PROPERTY_CUSTOM_PREFIX(capital, is);
		const fixed_point_t PROPERTY(colonial_points);
		const bool PROPERTY_CUSTOM_PREFIX(build_overseas, can);
		const uint32_t PROPERTY(min_port_level);
		const int32_t PROPERTY(limit_per_port);
		const fixed_point_t PROPERTY(supply_consumption_score);

		const fixed_point_t PROPERTY(hull);
		const fixed_point_t PROPERTY(gun_power);
		const fixed_point_t PROPERTY(fire_range);
		const fixed_point_t PROPERTY(evasion);
		const fixed_point_t PROPERTY(torpedo_attack);

		UnitTypeBranched(std::string_view new_identifier, unit_type_args_t& unit_args, ship_type_args_t const& ship_type_args);

	public:
		UnitTypeBranched(UnitTypeBranched&&) = default;
	};

	using ShipType = UnitTypeBranched<UnitType::branch_t::NAVAL>;

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
			NodeTools::Callback<UnitType::branch_t> auto callback
		) {
			using enum UnitType::branch_t;
			static const string_map_t<UnitType::branch_t> branch_map {
				{ "land", LAND }, { "naval", NAVAL }, { "sea", NAVAL }
			};
			return NodeTools::expect_mapped_string(branch_map, callback);
		}
		static NodeTools::NodeCallback auto expect_branch_identifier(NodeTools::Callback<UnitType::branch_t> auto callback) {
			return NodeTools::expect_identifier(expect_branch_str(callback));
		}

		bool load_unit_type_file(
			GoodDefinitionManager const& good_definition_manager, TerrainTypeManager const& terrain_type_manager,
			ModifierManager const& modifier_manager, ast::NodeCPtr root
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
