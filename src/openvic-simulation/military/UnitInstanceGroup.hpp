#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	struct ProvinceInstance;
	struct CountryInstance;
	struct MapInstance;

	struct UnitInstanceGroup {
	private:
		const unique_id_t PROPERTY(unique_id);
		const UnitType::branch_t PROPERTY(branch);
		memory::string PROPERTY(name);
		memory::vector<UnitInstance*> PROPERTY(units);
		LeaderInstance* PROPERTY_PTR(leader, nullptr);
		ProvinceInstance* PROPERTY_PTR(position, nullptr);
		CountryInstance* PROPERTY_PTR(country, nullptr);

		fixed_point_t PROPERTY(total_organisation);
		fixed_point_t PROPERTY(total_max_organisation);
		fixed_point_t PROPERTY(total_strength);
		fixed_point_t PROPERTY(total_max_strength);

		// Movement attributes
		// Ordered list of provinces making up the path the unit is trying to move along,
		// the front province should always be adjacent to the unit's current position.
		memory::vector<ProvinceInstance*> PROPERTY(path);
		// Measured in distance travelled, increases each day by unit speed (after modifiers have been applied) until
		// it reaches the required distance/movement cost to move to the next province in the path.
		fixed_point_t PROPERTY(movement_progress);

	protected:
		UnitInstanceGroup(
			unique_id_t new_unique_id,
			UnitType::branch_t new_branch,
			std::string_view new_name
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

		bool add_unit(UnitInstance& unit);
		bool remove_unit(UnitInstance const& unit);

		void set_name(std::string_view new_name);
		bool set_position(ProvinceInstance* new_position);
		bool set_country(CountryInstance* new_country);
		bool set_leader(LeaderInstance* new_leader);

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

	using ArmyInstance = UnitInstanceGroupBranched<UnitType::branch_t::LAND>;
	using NavyInstance = UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>;

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::LAND> : UnitInstanceGroup {
		friend struct UnitInstanceManager;

		using dig_in_level_t = uint8_t;

	private:
		NavyInstance* PROPERTY_PTR(transport_navy, nullptr);
		dig_in_level_t PROPERTY(dig_in_level, 0);

		bool PROPERTY_RW_CUSTOM_NAME(exiled, is_exiled, set_is_exiled, false);

		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		// TODO - do these work fine when units is empty?
		std::span<RegimentInstance* const> get_regiment_instances() {
			return { reinterpret_cast<RegimentInstance* const*>(get_units().data()), get_units().size() };
		}
		std::span<RegimentInstance const* const> get_regiment_instances() const {
			return { reinterpret_cast<RegimentInstance const* const*>(get_units().data()), get_units().size() };
		}
	};

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::NAVAL> : UnitInstanceGroup {
		friend struct UnitInstanceManager;

	private:
		memory::vector<ArmyInstance*> PROPERTY(carried_armies);

		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		std::span<ShipInstance* const> get_ship_instances() {
			return { reinterpret_cast<ShipInstance* const*>(get_units().data()), get_units().size() };
		}
		std::span<ShipInstance const* const> get_ship_instances() const {
			return { reinterpret_cast<ShipInstance const* const*>(get_units().data()), get_units().size() };
		}

		fixed_point_t get_total_consumed_supply() const;
	};

	template<UnitType::branch_t>
	struct UnitDeployment;

	template<UnitType::branch_t>
	struct UnitDeploymentGroup;

	struct MapInstance;
	struct Deployment;
	struct CultureManager;
	struct LeaderTraitManager;
	struct MilitaryDefines;

	struct UnitInstanceManager {
	private:
		// Used for leader pictures and names
		CultureManager const& culture_manager;
		LeaderTraitManager const& leader_trait_manager;
		MilitaryDefines const& military_defines;

		// TODO - use single counter or separate for leaders vs units vs unit groups? (even separate for branches?)
		// Starts at 1, so ID 0 represents an invalid value
		unique_id_t unique_id_counter = 1;

		// TODO - maps from unique_ids to leader/unit/unit group pointers (one big map or multiple maps?)

		memory::colony<LeaderInstance> PROPERTY(leaders);
		ordered_map<unique_id_t, LeaderInstance*> PROPERTY(leader_instance_map);

		memory::colony<RegimentInstance> PROPERTY(regiments);
		memory::colony<ShipInstance> PROPERTY(ships);
		ordered_map<unique_id_t, UnitInstance*> PROPERTY(unit_instance_map);

		UNIT_BRANCHED_GETTER(get_unit_instances, regiments, ships);

		memory::colony<ArmyInstance> PROPERTY(armies);
		memory::colony<NavyInstance> PROPERTY(navies);
		ordered_map<unique_id_t, UnitInstanceGroup*> PROPERTY(unit_instance_group_map);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);

		template<UnitType::branch_t Branch>
		UnitInstanceBranched<Branch>& generate_unit_instance(UnitDeployment<Branch> const& unit_deployment);
		template<UnitType::branch_t Branch>
		bool generate_unit_instance_group(
			MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
		);
		void generate_leader(CountryInstance& country, LeaderBase const& leader);

	public:
		UnitInstanceManager(
			CultureManager const& new_culture_manager,
			LeaderTraitManager const& new_leader_trait_manager,
			MilitaryDefines const& new_military_defines
		);

		bool generate_deployment(MapInstance& map_instance, CountryInstance& country, Deployment const* deployment);

		void update_gamestate();
		void tick();

		LeaderInstance* get_leader_instance_by_unique_id(unique_id_t unique_id);
		UnitInstance* get_unit_instance_by_unique_id(unique_id_t unique_id);
		UnitInstanceGroup* get_unit_instance_group_by_unique_id(unique_id_t unique_id);

		// Creates a new leader of the specified branch and adds it to the specified country. The leader's name and traits
		// can be specified, but if they are not, the leader will be generated with a random name and traits. The country's
		// leadership points will be checked and, if there are enough, have the leader creation cost subtracted from them.
		// If the country does not have enough leadership points, the function will return false and no leader will be created.
		bool create_leader(
			CountryInstance& country,
			UnitType::branch_t branch,
			Date creation_date,
			std::string_view name = {},
			LeaderTrait const* personality = nullptr,
			LeaderTrait const* background = nullptr
		);
	};
}
