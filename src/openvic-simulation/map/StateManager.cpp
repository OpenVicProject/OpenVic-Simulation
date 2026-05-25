#include "StateManager.hpp"

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/population/PopType.hpp"

using namespace OpenVic;

// Whether two provinces in the same region should be grouped into the same state or not.
static bool provinces_belong_in_same_state(ProvinceInstance const& lhs, ProvinceInstance const& rhs) {
	return lhs.get_owner() == rhs.get_owner() && lhs.get_colony_status() == rhs.get_colony_status();
}

bool StateManager::add_state_set(
	MapInstance& map_instance, Region const& region,
	PopsAggregateDeps const& pops_aggregate_deps,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<const PopType> pop_type_keys
) {
	OV_ERR_FAIL_COND_V_MSG(region.is_meta, false, memory::fmt::format("Cannot use meta region \"{}\" as state template!", region));
	OV_ERR_FAIL_COND_V_MSG(region.empty(), false, memory::fmt::format("Cannot use empty region \"{}\" as state template!", region));

	memory::vector<memory::vector<std::reference_wrapper<ProvinceInstance>>> temp_provinces;

	for (ProvinceDefinition const& province : region.get_provinces()) {
		ProvinceInstance& province_instance = map_instance.get_province_instance_by_definition(province);

		// add to existing state if shared owner & status...
		for (memory::vector<std::reference_wrapper<ProvinceInstance>>& provinces : temp_provinces) {
			if (provinces_belong_in_same_state(provinces.front(), province_instance)) {
				provinces.push_back(province_instance);
				// jump to the end of the outer loop, skipping the new state code
				goto loop_end;
			}
		}

		// ...otherwise start a new state
		temp_provinces.push_back({ province_instance });

	loop_end:;
		/* Either the province was added to an existing state and the program jumped to here,
		 * or it was used to create a new state and the program arrived here normally. */
	}

	state_sets.emplace_back(region);

	StateSet& state_set = state_sets.back();

	// Reserve space for the maximum number of states (one per province)
	state_set.states.reserve(region.size());

	for (memory::vector<std::reference_wrapper<ProvinceInstance>>& provinces : temp_provinces) {
		ProvinceInstance& capital = provinces.front();

		State& state = *state_set.states.emplace(
			/* TODO: capital province logic */
			state_set, &capital,
			std::move(provinces), capital.get_colony_status(),
			pops_aggregate_deps
		);

		for (ProvinceInstance& province : state.get_provinces()) {
			province.set_state(&state);
		}
	}

	return true;
}

bool StateManager::generate_states(
	MapDefinition const& map_definition,
	MapInstance& map_instance,
	PopsAggregateDeps const& pops_aggregate_deps,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<const PopType> pop_type_keys
) {
	state_sets.clear();
	state_sets.reserve(map_definition.get_region_count());

	bool ret = true;
	size_t state_count = 0;

	for (Region const& region : map_definition.get_regions()) {
		if (!region.is_meta) {
			if (add_state_set(
				map_instance,
				region,
				pops_aggregate_deps,
				strata_keys,
				pop_type_keys
			)) {
				state_count += state_sets.back().get_state_count();
			} else {
				ret = false;
			}
		}
	}

	SPDLOG_INFO("Generated {} states across {} state sets.", state_count, state_sets.size());

	return ret;
}

void StateManager::reset() {
	state_sets.clear();
}

void StateManager::update_gamestate() {
	for (StateSet& state_set : state_sets) {
		state_set.update_gamestate();
	}
}