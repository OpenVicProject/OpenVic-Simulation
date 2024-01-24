#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct TerrainType;
	struct TerrainTypeManager;

	struct Unit : HasIdentifier {
		using icon_t = uint32_t;
		using terrain_modifiers_t = ordered_map<TerrainType const*, ModifierValue>;

		enum struct branch_t : uint8_t { INVALID_BRANCH, LAND, NAVAL };
		enum struct unit_type_t : uint8_t {
			INVALID_UNIT_TYPE, INFANTRY, CAVALRY, SUPPORT, SPECIAL, BIG_SHIP, LIGHT_SHIP, TRANSPORT
		};

		struct unit_args_t {
			icon_t icon = 0;
			unit_type_t unit_type = unit_type_t::INVALID_UNIT_TYPE;
			// TODO defaults for move_sound and select_sound
			std::string_view sprite, move_sound, select_sound;
			bool active = true, floating_flag = false;
			uint32_t priority = 0;
			fixed_point_t max_strength = 0, default_organisation = 0, maximum_speed = 0, weighted_value = 0,
				supply_consumption = 0;
			Timespan build_time;
			Good::good_map_t build_cost, supply_cost;
			terrain_modifiers_t terrain_modifiers;

			unit_args_t() = default;
			unit_args_t(unit_args_t&&) = default;
		};

	private:
		const branch_t PROPERTY(branch); /* type in defines */
		const icon_t PROPERTY(icon);
		std::string PROPERTY(sprite);
		const bool PROPERTY_CUSTOM_PREFIX(active, is);
		unit_type_t PROPERTY(unit_type);
		const bool PROPERTY_CUSTOM_PREFIX(floating_flag, has);

		const uint32_t PROPERTY(priority);
		const fixed_point_t PROPERTY(max_strength);
		const fixed_point_t PROPERTY(default_organisation);
		const fixed_point_t PROPERTY(maximum_speed);
		const fixed_point_t PROPERTY(weighted_value);

		std::string PROPERTY(move_sound);
		std::string PROPERTY(select_sound);

		const Timespan PROPERTY(build_time);
		Good::good_map_t PROPERTY(build_cost);
		const fixed_point_t PROPERTY(supply_consumption);
		Good::good_map_t PROPERTY(supply_cost);

		terrain_modifiers_t PROPERTY(terrain_modifiers);

	protected:
		/* Non-const reference unit_args so variables can be moved from it. */
		Unit(std::string_view new_identifier, branch_t new_branch, unit_args_t& unit_args);

	public:
		Unit(Unit&&) = default;
	};

	struct LandUnit : Unit {
		friend struct UnitManager;

		enum struct allowed_cultures_t { ALL_CULTURES, ACCEPTED_CULTURES, PRIMARY_CULTURE };

		struct land_unit_args_t {
			allowed_cultures_t allowed_cultures = allowed_cultures_t::ALL_CULTURES;
			std::string_view sprite_override, sprite_mount, sprite_mount_attach_node;
			// TODO - represent these as modifier effects, so that they can be combined with tech, inventions,
			// leader bonuses, etc. and applied to unit instances all in one go (same for NavalUnits below)
			fixed_point_t reconnaissance = 0, attack = 0, defence = 0, discipline = 0, support = 0, maneuver = 0,
				siege = 0;

			land_unit_args_t() = default;
			land_unit_args_t(land_unit_args_t&&) = default;
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

		LandUnit(std::string_view new_identifier, unit_args_t& unit_args, land_unit_args_t const& land_unit_args);

	public:
		LandUnit(LandUnit&&) = default;
	};

	struct NavalUnit : Unit {
		friend struct UnitManager;

		struct naval_unit_args_t {
			Unit::icon_t naval_icon = 0;
			bool sail = false, transport = false, capital = false, build_overseas = false;
			uint32_t min_port_level = 0;
			int32_t limit_per_port = 0;
			fixed_point_t colonial_points = 0, supply_consumption_score = 0, hull = 0, gun_power = 0, fire_range = 0,
				evasion = 0, torpedo_attack = 0;

			naval_unit_args_t() = default;
			naval_unit_args_t(naval_unit_args_t&&) = default;
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

		NavalUnit(std::string_view new_identifier, unit_args_t& unit_args, naval_unit_args_t const& naval_unit_args);

	public:
		NavalUnit(NavalUnit&&) = default;
	};

	struct UnitManager {
	private:
		IdentifierPointerRegistry<Unit> IDENTIFIER_REGISTRY(unit);
		IdentifierRegistry<LandUnit> IDENTIFIER_REGISTRY(land_unit);
		IdentifierRegistry<NavalUnit> IDENTIFIER_REGISTRY(naval_unit);

	public:
		void reserve_all_units(size_t size);
		void lock_all_units();

		bool add_land_unit(
			std::string_view identifier, Unit::unit_args_t& unit_args, LandUnit::land_unit_args_t const& land_unit_args
		);
		bool add_naval_unit(
			std::string_view identifier, Unit::unit_args_t& unit_args, NavalUnit::naval_unit_args_t const& naval_unit_args
		);

		static NodeTools::Callback<std::string_view> auto expect_branch_str(NodeTools::Callback<Unit::branch_t> auto callback) {
			using enum Unit::branch_t;
			static const string_map_t<Unit::branch_t> branch_map { { "land", LAND }, { "naval", NAVAL }, { "sea", NAVAL } };
			return NodeTools::expect_mapped_string(branch_map, callback);
		}
		static NodeTools::NodeCallback auto expect_branch_identifier(NodeTools::Callback<Unit::branch_t> auto callback) {
			return NodeTools::expect_identifier(expect_branch_str(callback));
		}

		bool load_unit_file(
			GoodManager const& good_manager, TerrainTypeManager const& terrain_type_manager,
			ModifierManager const& modifier_manager, ast::NodeCPtr root
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;
	};
}
