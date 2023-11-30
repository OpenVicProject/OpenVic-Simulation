#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#define UNIT_PARAMS \
	Unit::icon_t icon, std::string_view sprite, bool active, std::string_view unit_type, bool floating_flag, \
	uint32_t priority, fixed_point_t max_strength, fixed_point_t default_organisation, fixed_point_t maximum_speed, \
	fixed_point_t weighted_value, std::string_view move_sound, std::string_view select_sound, Timespan build_time, \
	Good::good_map_t&& build_cost, fixed_point_t supply_consumption, Good::good_map_t&& supply_cost

#define LAND_PARAMS \
	bool primary_culture, std::string_view sprite_override, std::string_view sprite_mount, \
	std::string_view sprite_mount_attach_node, fixed_point_t reconnaissance, fixed_point_t attack, fixed_point_t defence, \
	fixed_point_t discipline, fixed_point_t support, fixed_point_t maneuver, fixed_point_t siege

#define NAVY_PARAMS \
	Unit::icon_t naval_icon, bool sail, bool transport, bool capital, fixed_point_t colonial_points, bool build_overseas, \
	uint32_t min_port_level, int32_t limit_per_port, fixed_point_t supply_consumption_score, fixed_point_t hull, \
	fixed_point_t gun_power, fixed_point_t fire_range, fixed_point_t evasion, fixed_point_t torpedo_attack

namespace OpenVic {
	struct Unit : HasIdentifier {
		using icon_t = uint32_t;

		enum struct type_t { LAND, NAVAL };

	private:
		const type_t PROPERTY(type);
		const icon_t PROPERTY(icon);
		const std::string PROPERTY(sprite);
		const bool PROPERTY_CUSTOM_PREFIX(active, is);
		const std::string PROPERTY(unit_type);
		const bool PROPERTY_CUSTOM_PREFIX(floating_flag, has);

		const uint32_t PROPERTY(priority);
		const fixed_point_t PROPERTY(max_strength);
		const fixed_point_t PROPERTY(default_organisation);
		const fixed_point_t PROPERTY(maximum_speed);
		const fixed_point_t PROPERTY(weighted_value);

		const std::string PROPERTY(move_sound);
		const std::string PROPERTY(select_sound);

		const Timespan PROPERTY(build_time);
		const Good::good_map_t PROPERTY(build_cost);
		const fixed_point_t PROPERTY(supply_consumption);
		const Good::good_map_t PROPERTY(supply_cost);

	protected:
		Unit(std::string_view identifier, type_t type, UNIT_PARAMS);

	public:
		Unit(Unit&&) = default;
	};

	struct LandUnit : Unit {
		friend struct UnitManager;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(primary_culture, is);
		const std::string PROPERTY(sprite_override);
		const std::string PROPERTY(sprite_mount);
		const std::string PROPERTY(sprite_mount_attach_node);
		const fixed_point_t PROPERTY(reconnaissance);
		const fixed_point_t PROPERTY(attack);
		const fixed_point_t PROPERTY(defence);
		const fixed_point_t PROPERTY(discipline);
		const fixed_point_t PROPERTY(support);
		const fixed_point_t PROPERTY(maneuver);
		const fixed_point_t PROPERTY(siege);

		LandUnit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS);

	public:
		LandUnit(LandUnit&&) = default;
	};

	struct NavalUnit : Unit {
		friend struct UnitManager;

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

		NavalUnit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS);

	public:
		NavalUnit(NavalUnit&&) = default;
	};

	struct UnitManager {
	private:
		IdentifierRegistry<Unit> units;

		bool _check_shared_parameters(std::string_view identifier, UNIT_PARAMS);

	public:
		UnitManager();

		bool add_land_unit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS);
		bool add_naval_unit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS);
		IDENTIFIER_REGISTRY_ACCESSORS(unit)

		static NodeTools::callback_t<std::string_view> expect_type_str(NodeTools::Callback<Unit::type_t> auto callback);

		bool load_unit_file(GoodManager const& good_manager, ast::NodeCPtr root);
		bool generate_modifiers(ModifierManager& modifier_manager);
	};
}
