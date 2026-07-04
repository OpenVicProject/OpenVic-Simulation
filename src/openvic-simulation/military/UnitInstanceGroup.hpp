#pragma once

#include <functional>
#include <string_view>

#include "openvic-simulation/core/memory/Colony.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp" //below other imports that undef the macros

namespace OpenVic {
	struct ProvinceInstance;
	struct CountryInstance;
	struct MapInstance;

	struct UnitInstanceGroup {
	private:
		memory::string PROPERTY(name);
		memory::vector<std::reference_wrapper<UnitInstance>> SPAN_PROPERTY(units);
		LeaderInstance* PROPERTY_PTR(leader, nullptr);
		std::reference_wrapper<ProvinceInstance> PROPERTY(location);
		std::reference_wrapper<CountryInstance> PROPERTY(country);

		fixed_point_t PROPERTY(total_organisation);
		fixed_point_t PROPERTY(total_max_organisation);
		fixed_point_t PROPERTY(total_strength);
		fixed_point_t PROPERTY(total_max_strength);

		// Movement attributes
		// Ordered list of provinces making up the path the unit is trying to move along,
		// the front province should always be adjacent to the unit's current location.
		memory::vector<std::reference_wrapper<ProvinceInstance>> SPAN_PROPERTY(path);
		// Measured in distance travelled, increases each day by unit speed (after modifiers have been applied) until
		// it reaches the required distance/movement cost to move to the next province in the path.
		fixed_point_t PROPERTY(movement_progress);

	protected:
		UnitInstanceGroup(
			unique_id_t new_unique_id,
			unit_branch_t new_branch,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);

		void update_gamestate();
		void tick();

	public:
		const unique_id_t unique_id;
		const unit_branch_t branch;

		UnitInstanceGroup(UnitInstanceGroup&&) = default;
		UnitInstanceGroup(UnitInstanceGroup const&) = delete;

		size_t get_unit_count() const;
		bool empty() const;
		size_t get_unit_category_count(UnitType::unit_category_t unit_category) const;
		UnitType const* get_display_unit_type() const;

		bool add_unit(UnitInstance& unit);
		bool remove_unit(UnitInstance const& unit);

		void set_name(std::string_view new_name);
		bool set_location(ProvinceInstance& new_location);
		bool set_country(CountryInstance& new_country);
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

		constexpr bool operator==(UnitInstanceGroup const& rhs) const {
			return unique_id == rhs.unique_id;
		}
	};

	template<>
	struct UnitInstanceGroupBranched<unit_branch_t::LAND> : UnitInstanceGroup {
		using dig_in_level_t = uint8_t;

	private:
		NavyInstance* PROPERTY_PTR(transport_navy, nullptr);
		dig_in_level_t PROPERTY(dig_in_level, 0);

		bool PROPERTY_RW_CUSTOM_NAME(exiled, is_exiled, set_is_exiled, false);

	public:
		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		// TODO - do these work fine when units is empty?
		std::span<const std::reference_wrapper<RegimentInstance>> get_regiment_instances() {
			return { reinterpret_cast<std::reference_wrapper<RegimentInstance> const*>(get_units().data()), get_units().size() };
		}
		std::span<const std::reference_wrapper<const RegimentInstance>> get_regiment_instances() const {
			return { reinterpret_cast<std::reference_wrapper<const RegimentInstance> const*>(get_units().data()), get_units().size() };
		}
	};

	template<>
	struct UnitInstanceGroupBranched<unit_branch_t::NAVAL> : UnitInstanceGroup {
	private:
		memory::vector<std::reference_wrapper<ArmyInstance>> SPAN_PROPERTY(carried_armies);

	public:
		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		std::span<const std::reference_wrapper<ShipInstance>> get_ship_instances() {
			return { reinterpret_cast<std::reference_wrapper<ShipInstance> const*>(get_units().data()), get_units().size() };
		}
		std::span<const std::reference_wrapper<const ShipInstance>> get_ship_instances() const {
			return { reinterpret_cast<std::reference_wrapper<const ShipInstance> const*>(get_units().data()), get_units().size() };
		}

		fixed_point_t get_total_consumed_supply() const;
	};

	template<unit_branch_t>
	struct UnitDeployment;

	template<unit_branch_t>
	struct UnitDeploymentGroup;

	struct MapInstance;
	struct Deployment;
	struct CultureManager;
	struct LeaderTraitManager;
	struct MilitaryDefines;
	struct Pop;

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
		ordered_map<unique_id_t, std::reference_wrapper<LeaderInstance>> PROPERTY(leader_instance_map);

		memory::colony<RegimentInstance> PROPERTY(regiments);
		memory::colony<ShipInstance> PROPERTY(ships);
		ordered_map<unique_id_t, std::reference_wrapper<UnitInstance>> PROPERTY(unit_instance_map);

		OV_UNIT_BRANCHED_GETTER(get_unit_instances, regiments, ships);

		memory::colony<ArmyInstance> PROPERTY(armies);
		memory::colony<NavyInstance> PROPERTY(navies);
		ordered_map<unique_id_t, std::reference_wrapper<UnitInstanceGroup>> PROPERTY(unit_instance_group_map);

		OV_UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);

		Pop* recruit_pop_in(ProvinceInstance& province, const bool is_rebel) const;
		template<unit_branch_t Branch>
		UnitInstanceBranched<Branch>& generate_unit_instance(
			UnitDeployment<Branch> const& unit_deployment,
			MapInstance& map_instance,
			const bool is_rebel
		);
		template<unit_branch_t Branch>
		bool generate_unit_instance_group(
			MapInstance& map_instance, CountryInstance& country, UnitDeploymentGroup<Branch> const& unit_deployment_group
		);
		template<typename T>
		void generate_leader(CountryInstance& country, T&& leader_base);

	public:
		UnitInstanceManager(
			CultureManager const& new_culture_manager,
			LeaderTraitManager const& new_leader_trait_manager,
			MilitaryDefines const& new_military_defines
		);

		bool generate_deployment(MapInstance& map_instance, CountryInstance& country, Deployment const& deployment);

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
			unit_branch_t branch,
			Date creation_date,
			std::string_view name = {},
			LeaderTrait const* personality = nullptr,
			LeaderTrait const* background = nullptr
		);
		//this seems too minimal, am i missing something?
		bool create_regiment(
			RegimentType const& regiment_type,
			Pop& pop,
			ProvinceInstance& location
		);

	};
}
