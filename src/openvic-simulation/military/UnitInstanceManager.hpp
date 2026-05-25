#pragma once

#include <functional>
#include <string_view>

#include "openvic-simulation/core/memory/Colony.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp" //below other imports that undef the macros

namespace OpenVic {
	struct MapInstance;
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
	};
}
