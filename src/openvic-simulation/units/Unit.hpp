#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

#define UNIT_PARAMS Unit::icon_t icon, Unit::sprite_t sprite, bool active, UnitType type, bool floating_flag, uint32_t priority,\
					uint32_t max_strength, uint32_t default_organisation,uint32_t maximum_speed, fixed_point_t weighted_value, \
					uint32_t build_time, std::map<const Good*, fixed_point_t> build_cost, fixed_point_t supply_consumption, \
					std::map<const Good*, fixed_point_t> supply_cost, uint32_t supply_consumption_score
#define LAND_PARAMS uint32_t reconnaisance, uint32_t attack, uint32_t defence, uint32_t discipline, uint32_t support, \
					uint32_t maneuver, uint32_t siege
#define NAVY_PARAMS Unit::icon_t naval_icon, bool sail, bool transport, Unit::sound_t move_sound, Unit::sound_t select_sound, \
					uint32_t colonial_points, bool build_overseas, uint32_t min_port_level, int32_t limit_per_port, \
					uint32_t hull, uint32_t gun_power, fixed_point_t fire_range, uint32_t evasion, uint32_t torpedo_attack

namespace OpenVic {
	enum UnitCategory {
		NAVAL,
		LAND
	};

	enum UnitType {
		INFANTRY, //guard, infantry, irregular
		CAVALRY, //cavalry, cuirassier, dragoon, hussar, plane
		SUPPORT, //artillery
		SPECIAL, //engineer, tank
		TRANSPORT, //clipper transport, steam transport
		LIGHT_SHIP, //commerce raider, cruiser, frigate
		BIG_SHIP //battleship, dreadnought, ironclad, manowar, monitor
	};

	struct Unit : HasIdentifier {
		using icon_t = uint32_t;
		using sprite_t = std::string_view;
		using sound_t = std::string_view;

	private:
		const icon_t icon;
		const UnitCategory category; //type
		const sprite_t sprite;
		const bool active;
		const UnitType type; //unit_type
		const bool floating_flag;

		const uint32_t priority;
		const uint32_t max_strength;
		const uint32_t default_organisation;
		const uint32_t maximum_speed;
		const fixed_point_t weighted_value;

		const uint32_t build_time;
		const std::map<const Good*, fixed_point_t> build_cost;
		const fixed_point_t supply_consumption;
		const std::map<const Good*, fixed_point_t> supply_cost;
		const uint32_t supply_consumption_score;

	protected:
		Unit(std::string_view identifier, UnitCategory category, UNIT_PARAMS);
	
	public:
		Unit(Unit&&) = default;

		icon_t get_icon() const;
		UnitCategory get_category() const;
		sprite_t get_sprite() const;
		bool is_active() const;
		UnitType get_type() const;
		bool has_floating_flag() const;

		uint32_t get_priority() const;
		uint32_t get_max_strength() const;
		uint32_t get_default_organisation() const;
		uint32_t get_maximum_speed() const;
		fixed_point_t get_weighted_value() const;

		uint32_t get_build_time() const;
		std::map<const Good*, fixed_point_t> get_build_cost() const;
		fixed_point_t get_supply_consumption() const;
		std::map<const Good*, fixed_point_t> get_supply_cost() const;
		uint32_t get_supply_consumption_score() const;
	};

	struct LandUnit : Unit {
		friend struct UnitManager;

	private:
		const uint32_t reconnaisance;
		const uint32_t attack;
		const uint32_t defence;
		const uint32_t discipline;
		const uint32_t support;
		const uint32_t maneuver;
		const uint32_t siege;

		LandUnit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS);

	public:
		LandUnit(LandUnit&&) = default;

		uint32_t get_reconnaisance() const;
		uint32_t get_attack() const;
		uint32_t get_defence() const;
		uint32_t get_discipline() const;
		uint32_t get_support() const;
		uint32_t get_maneuver() const;
		uint32_t get_siege() const;
	};

	struct NavalUnit : Unit {
		friend struct UnitManager;
	
	private:
		const icon_t naval_icon;
		const bool sail;
		const bool transport;
		const sound_t move_sound;
		const sound_t select_sound;
		const uint32_t colonial_points; //some
		const bool build_overseas; //can_build_overseas
		const uint32_t min_port_level;
		const int32_t limit_per_port; //-1 is unlimited

		const uint32_t hull;
		const uint32_t gun_power;
		const fixed_point_t fire_range;
		const uint32_t evasion;
		const uint32_t torpedo_attack;

		NavalUnit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS);

	public:
		NavalUnit(NavalUnit&&) = default;

		icon_t get_naval_icon() const;
		bool can_sail() const;
		bool is_transport() const;
		sound_t get_move_sound() const;
		sound_t get_select_sound() const;
		uint32_t get_colonial_points() const;
		bool can_build_overseas() const;
		uint32_t get_min_port_level() const;
		int32_t get_limit_per_port() const;

		uint32_t get_hull() const;
		uint32_t get_gun_power() const;
		fixed_point_t get_fire_range() const;
		uint32_t get_evasion() const;
		uint32_t get_torpedo_attack() const;
	};

	struct UnitManager {
	private:
		IdentifierRegistry<Unit> units;

		bool _check_superclass_parameters(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS);
	
	public:
		UnitManager();

		bool add_land_unit(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS, LAND_PARAMS);
		bool add_naval_unit(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS, NAVY_PARAMS);
		IDENTIFIER_REGISTRY_ACCESSORS(Unit, unit)

		bool load_unit_file(ast::NodeCPtr root);
	};
}