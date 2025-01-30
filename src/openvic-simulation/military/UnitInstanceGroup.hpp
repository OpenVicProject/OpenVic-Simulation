#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	template<UnitType::branch_t>
	struct LeaderBranched;

	struct ProvinceInstance;
	struct CountryInstance;
	struct MapInstance;

	template<UnitType::branch_t Branch>
	struct UnitInstanceGroup {
		using _UnitInstance = UnitInstanceBranched<Branch>;
		using _Leader = LeaderBranched<Branch>;

	private:
		std::string PROPERTY(name);
		std::vector<_UnitInstance*> PROPERTY(units);
		_Leader* PROPERTY_PTR(leader, nullptr);

		fixed_point_t PROPERTY(total_organisation);
		fixed_point_t PROPERTY(total_max_organisation);
		fixed_point_t PROPERTY(total_strength);
		fixed_point_t PROPERTY(total_max_strength);

		// Movement attributes
		// Ordered list of provinces making up the path the unit is trying to move along,
		// the front province should always be adjacent to the unit's current position.
		std::vector<ProvinceInstance*> PROPERTY(path);
		// Measured in distance travelled, increases each day by unit speed (after modifiers have been applied) until
		// it reaches the required distance/movement cost to move to the next province in the path.
		fixed_point_t PROPERTY(movement_progress);

	protected:
		ProvinceInstance* PROPERTY_PTR_ACCESS(position, protected, nullptr);
		CountryInstance* PROPERTY_PTR_ACCESS(country, protected, nullptr);

		UnitInstanceGroup(
			std::string_view new_name,
			std::vector<_UnitInstance*>&& new_units
		);

		void update_gamestate();
		void tick();

	public:
		UnitInstanceGroup(UnitInstanceGroup&&) = default;
		UnitInstanceGroup(UnitInstanceGroup const&) = delete;

		size_t get_unit_count() const;
		bool empty() const;
		size_t get_unit_category_count(UnitType::unit_category_t unit_category) const;
		UnitType const* get_display_unit_type() const;

		void set_name(std::string_view new_name);
		bool set_position(ProvinceInstance* new_position);
		bool set_country(CountryInstance* new_country);
		bool set_leader(_Leader* new_leader);

		fixed_point_t get_organisation_proportion() const;
		fixed_point_t get_strength_proportion() const;

		fixed_point_t get_average_organisation() const;
		fixed_point_t get_average_max_organisation() const;

		bool is_moving() const;
		// The adjacent province that the unit will arrive in next, not necessarily the final destination of its current path
		ProvinceInstance const* get_movement_destination_province() const;
		Date get_movement_arrival_date() const;

		bool is_in_combat() const;
	};

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::LAND> : UnitInstanceGroup<UnitType::branch_t::LAND> {
		friend struct UnitInstanceManager;

		using dig_in_level_t = uint8_t;

	private:
		dig_in_level_t PROPERTY(dig_in_level, 0);

		UnitInstanceGroupBranched(
			std::string_view new_name,
			std::vector<RegimentInstance*>&& new_units
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();
	};

	using ArmyInstance = UnitInstanceGroupBranched<UnitType::branch_t::LAND>;

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::NAVAL> : UnitInstanceGroup<UnitType::branch_t::NAVAL> {
		friend struct UnitInstanceManager;

	private:
		std::vector<ArmyInstance*> PROPERTY(carried_armies);

		UnitInstanceGroupBranched(
			std::string_view new_name,
			std::vector<ShipInstance*>&& new_ships
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		fixed_point_t get_total_consumed_supply() const;
	};

	using NavyInstance = UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>;

	template<UnitType::branch_t>
	struct UnitDeployment;

	template<UnitType::branch_t>
	struct UnitDeploymentGroup;

	struct MapInstance;
	struct Deployment;
	struct CultureManager;
	struct LeaderBase;

	struct UnitInstanceManager {
	private:
		plf::colony<RegimentInstance> PROPERTY(regiments);
		plf::colony<ShipInstance> PROPERTY(ships);

		UNIT_BRANCHED_GETTER(get_unit_instances, regiments, ships);

		plf::colony<ArmyInstance> PROPERTY(armies);
		plf::colony<NavyInstance> PROPERTY(navies);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);

		template<UnitType::branch_t Branch>
		bool generate_unit_instance(
			UnitDeployment<Branch> const& unit_deployment, UnitInstanceBranched<Branch>*& unit_instance
		);
		template<UnitType::branch_t Branch>
		bool generate_unit_instance_group(
			MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
		);
		static bool generate_leader(CultureManager const& culture_manager, CountryInstance& country, LeaderBase const& leader);

	public:
		bool generate_deployment(
			CultureManager const& culture_manager, MapInstance& map_instance, CountryInstance& country,
			Deployment const* deployment
		);

		void update_gamestate();
		void tick();
	};
}
