#pragma once

#include <concepts>
#include <string_view>
#include <vector>
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	template<std::derived_from<UnitType> T>
	struct UnitInstance {
	protected:
		std::string PROPERTY(unit_name);
		T const& PROPERTY(unit_type); //can't change

		fixed_point_t PROPERTY_RW(organisation);
		fixed_point_t PROPERTY_RW(morale);
		fixed_point_t PROPERTY_RW(strength);

	protected:
		UnitInstance(std::string_view new_unit_name, T const& new_unit_type) :
			unit_name { new_unit_name },
			unit_type { new_unit_type },
			organisation { new_unit_type.get_default_organisation() }, //TODO: modifiers
			morale { 0 }, //TODO: modifiers
			strength { new_unit_type.get_max_strength() } {}
	public:
		void set_unit_name(std::string_view new_unit_name) {
			unit_name = new_unit_name;
		}
	};

	struct RegimentInstance : UnitInstance<RegimentType> {
	private:
		Pop& PROPERTY(pop);

	public:
		RegimentInstance(std::string_view new_name, RegimentType const& new_regiment_type, Pop& new_pop);
	};

	struct ShipInstance : UnitInstance<ShipType> {
	public:
		ShipInstance(std::string_view new_name, ShipType const& new_ship_type);
	};

	struct MovementInfo {
	private:
		std::vector<Province const*> PROPERTY(path);
		fixed_point_t PROPERTY(movement_progress);

	public:
		MovementInfo();
		MovementInfo(Province const* starting_province, Province const* target_province); // contains/calls pathfinding logic
	};

	template<utility::is_derived_from_specialization_of<UnitInstance> I>
	struct UnitInstanceGroup {
	private:
		std::string PROPERTY(name);
		const UnitType::branch_t PROPERTY(branch);
		std::vector<I*> PROPERTY(units);
		Leader const* PROPERTY_RW(leader);
		Province const* PROPERTY_RW(position);

		MovementInfo PROPERTY_REF(movement_info);

	protected:
		UnitInstanceGroup(
			std::string_view new_name,
			UnitType::branch_t new_branch,
			std::vector<I*>&& new_units,
			Leader const* new_leader,
			Province const* new_position
		) : name { new_name },
			branch { new_branch },
			units { std::move(new_units) },
			leader { new_leader },
			position { new_position } {}
	
	public:
		void set_name(std::string_view new_name) {
			name = new_name;
		}
	};

	struct ArmyInstance : UnitInstanceGroup<RegimentInstance> {
	public:
		ArmyInstance(
			std::string_view new_name,
			std::vector<RegimentInstance*>&& new_units,
			Leader const* new_leader,
			Province const* new_position
		);
	};

	struct NavyInstance : UnitInstanceGroup<ShipInstance> {
	private:
		std::vector<ArmyInstance const*> PROPERTY(carried_armies);

	public:
		NavyInstance(
			std::string_view new_name,
			std::vector<ShipInstance*>&& new_ships,
			Leader const* new_leader,
			Province const* new_position
		);
	};
}