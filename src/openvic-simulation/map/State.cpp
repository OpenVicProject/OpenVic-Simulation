#include "State.hpp"

#include "openvic-simulation/map/Map.hpp"

using namespace OpenVic;

State::State(
	Country const* owner, Province const* capital, Region::provinces_t&& provinces, Province::colony_status_t colony_status
) : owner { owner }, capital { capital }, provinces { std::move(provinces) }, colony_status { colony_status } {}

/* Whether two provinces in the same region should be grouped into the same state or not.
 * (Assumes both provinces non-null.) */
static bool provinces_belong_in_same_state(Province const* lhs, Province const* rhs) {
	return lhs->get_owner() == rhs->get_owner() && lhs->get_colony_status() == rhs->get_colony_status();
}

StateSet::StateSet(Map& map, Region const& new_region) : region { new_region } {
	if (region.get_meta()) {
		Logger::error("Cannot use meta region as state template!");
	}

	std::vector<Region::provinces_t> temp_provinces;

	for (Province const* province : region.get_provinces()) {
		// add to existing state if shared owner & status...
		for (Region::provinces_t& provinces : temp_provinces) {
			if (provinces_belong_in_same_state(provinces[0], province)) {
				provinces.push_back(province);
				// jump to the end of the outer loop, skipping the new state code
				goto loop_end;
			}
		}
		// ...otherwise start a new state
		temp_provinces.push_back({ province });
	loop_end:;
		/* Either the province was added to an existing state and the program jumped to here,
		 * or it was used to create a new state and the program arrived here normally. */
	}

	for (Region::provinces_t& provinces : temp_provinces) {
		states.emplace_back(
			/* TODO: capital province logic */
			provinces[0]->get_owner(), provinces[0], std::move(provinces), provinces[0]->get_colony_status()
		);
	}

	// Go back and assign each new state to its provinces.
	for (State const& state : states) {
		for (Province const* province : state.get_provinces()) {
			map.remove_province_const(province)->set_state(&state);
		}
	}
}

StateSet::states_t& StateSet::get_states() {
	return states;
}

void StateManager::generate_states(Map& map) {
	regions.clear();
	regions.reserve(map.get_region_count());
	for (Region const& region : map.get_regions()) {
		if (!region.get_meta()) {
			regions.emplace_back(map, region);
		}
	}
	Logger::info("Generated states.");
}
