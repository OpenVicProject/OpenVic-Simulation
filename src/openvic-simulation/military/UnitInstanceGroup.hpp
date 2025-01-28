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
	struct ProvinceInstance;

	struct MovementInfo {
	private:
		std::vector<ProvinceInstance const*> PROPERTY(path);
		fixed_point_t PROPERTY(movement_progress);

	public:
		MovementInfo() = default;
		// contains/calls pathfinding logic
		MovementInfo(ProvinceInstance const* starting_province, ProvinceInstance const* target_province);
		MovementInfo(MovementInfo&&) = default;
	};

	template<UnitType::branch_t>
	struct LeaderBranched;

	struct CountryInstance;

	template<UnitType::branch_t Branch>
	struct UnitInstanceGroup {
		using _UnitInstance = UnitInstanceBranched<Branch>;
		using _Leader = LeaderBranched<Branch>;

	private:
		std::string PROPERTY(name);
		std::vector<_UnitInstance*> PROPERTY(units);
		_Leader* PROPERTY_PTR(leader, nullptr);

		MovementInfo PROPERTY_REF(movement_info);

	protected:
		ProvinceInstance* PROPERTY_PTR_ACCESS(position, protected, nullptr);
		CountryInstance* PROPERTY_PTR_ACCESS(country, protected, nullptr);

		UnitInstanceGroup(
			std::string_view new_name,
			std::vector<_UnitInstance*>&& new_units
		);

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
	};

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::LAND> : UnitInstanceGroup<UnitType::branch_t::LAND> {
		friend struct UnitInstanceManager;

	private:
		UnitInstanceGroupBranched(
			std::string_view new_name,
			std::vector<RegimentInstance*>&& new_units
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;
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
	};
}
