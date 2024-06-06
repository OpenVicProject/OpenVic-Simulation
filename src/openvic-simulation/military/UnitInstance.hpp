#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	template<std::derived_from<UnitType> T>
	struct UnitInstance {
	private:
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
		UnitInstance(UnitInstance&&) = default;

		void set_unit_name(std::string_view new_unit_name) {
			unit_name = new_unit_name;
		}
	};

	struct Pop;

	struct RegimentInstance : UnitInstance<RegimentType> {
		friend struct UnitInstanceManager;

	private:
		Pop* PROPERTY(pop);

		RegimentInstance(std::string_view new_name, RegimentType const& new_regiment_type, Pop* new_pop);

	public:
		RegimentInstance(RegimentInstance&&) = default;
	};

	struct ShipInstance : UnitInstance<ShipType> {
		friend struct UnitInstanceManager;

	private:
		ShipInstance(std::string_view new_name, ShipType const& new_ship_type);

	public:
		ShipInstance(ShipInstance&&) = default;
	};

	struct ProvinceInstance;

	struct MovementInfo {
	private:
		std::vector<ProvinceInstance const*> PROPERTY(path);
		fixed_point_t PROPERTY(movement_progress);

	public:
		MovementInfo();
		// contains/calls pathfinding logic
		MovementInfo(ProvinceInstance const* starting_province, ProvinceInstance const* target_province);
	};

	struct CountryInstance;

	template<utility::is_derived_from_specialization_of<UnitInstance> I>
	struct UnitInstanceGroup {
	private:
		std::string PROPERTY(name);
		const UnitType::branch_t PROPERTY(branch);
		std::vector<I*> PROPERTY(units);
		Leader const* PROPERTY(leader);

		MovementInfo PROPERTY_REF(movement_info);

	protected:
		ProvinceInstance* PROPERTY_ACCESS(position, protected);
		CountryInstance* PROPERTY_ACCESS(country, protected);

		UnitInstanceGroup(
			std::string_view new_name,
			UnitType::branch_t new_branch,
			std::vector<I*>&& new_units,
			Leader const* new_leader,
			CountryInstance* new_country
		) : name { new_name },
			branch { new_branch },
			units { std::move(new_units) },
			leader { new_leader },
			position { nullptr },
			country { new_country } {}

	public:
		UnitInstanceGroup(UnitInstanceGroup&&) = default;
		UnitInstanceGroup(UnitInstanceGroup const&) = delete;

		void set_name(std::string_view new_name) {
			name = new_name;
		}

		size_t get_unit_count() const {
			return units.size();
		}

		bool empty() const {
			return units.empty();
		}

		size_t get_unit_category_count(UnitType::unit_category_t unit_category) const {
			return std::count_if(units.begin(), units.end(), [unit_category](I const* unit) {
				return unit->unit_type.get_unit_category() == unit_category;
			});
		}

		UnitType const* get_display_unit_type() const {
			if (units.empty()) {
				return nullptr;
			}

			fixed_point_map_t<UnitType const*> weighted_unit_types;

			for (I const* unit : units) {
				UnitType const& unit_type = unit->get_unit_type();
				weighted_unit_types[&unit_type] += unit_type.get_weighted_value();
			}

			return get_largest_item_tie_break(
				weighted_unit_types,
				[](UnitType const* lhs, UnitType const* rhs) -> bool {
					return lhs->get_weighted_value() < rhs->get_weighted_value();
				}
			)->first;
		}

		virtual void set_position(ProvinceInstance* new_position) = 0;
	};

	struct ArmyInstance : UnitInstanceGroup<RegimentInstance> {
		friend struct UnitInstanceManager;

	private:
		ArmyInstance(
			std::string_view new_name,
			std::vector<RegimentInstance*>&& new_units,
			Leader const* new_leader,
			CountryInstance* new_country
		);

	public:
		ArmyInstance(ArmyInstance&&) = default;

		void set_position(ProvinceInstance* new_position) override;
	};

	struct NavyInstance : UnitInstanceGroup<ShipInstance> {
		friend struct UnitInstanceManager;

	private:
		std::vector<ArmyInstance const*> PROPERTY(carried_armies);

		NavyInstance(
			std::string_view new_name,
			std::vector<ShipInstance*>&& new_ships,
			Leader const* new_leader,
			CountryInstance* new_country
		);

	public:
		NavyInstance(NavyInstance&&) = default;

		void set_position(ProvinceInstance* new_position) override;
	};

	struct RegimentDeployment;
	struct ShipDeployment;
	struct Map;
	struct ArmyDeployment;
	struct NavyDeployment;
	struct Deployment;

	struct UnitInstanceManager {
	private:
		std::deque<RegimentInstance> PROPERTY(regiments);
		std::deque<ShipInstance> PROPERTY(ships);

		std::deque<ArmyInstance> PROPERTY(armies);
		std::deque<NavyInstance> PROPERTY(navies);

		bool generate_regiment(RegimentDeployment const& regiment_deployment, RegimentInstance*& regiment);
		bool generate_ship(ShipDeployment const& ship_deployment, ShipInstance*& ship);
		bool generate_army(Map& map, CountryInstance& country, ArmyDeployment const& army_deployment);
		bool generate_navy(Map& map, CountryInstance& country, NavyDeployment const& navy_deployment);

	public:
		bool generate_deployment(Map& map, CountryInstance& country, Deployment const* deployment);
	};
}
