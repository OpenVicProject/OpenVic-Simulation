#include "State.hpp"

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"

using namespace OpenVic;

State::State(
	StateSet const& new_state_set, Country const* owner, ProvinceInstance* capital, std::vector<ProvinceInstance*>&& provinces,
	ProvinceInstance::colony_status_t colony_status
) : state_set { new_state_set }, owner { owner }, capital { capital }, provinces { std::move(provinces) },
	colony_status { colony_status } {}

void State::update_gamestate() {
	total_population = 0;

	for (ProvinceInstance const* province : provinces) {
		total_population += province->get_total_population();
	}
}

/* Whether two provinces in the same region should be grouped into the same state or not.
 * (Assumes both provinces non-null.) */
static bool provinces_belong_in_same_state(ProvinceInstance const* lhs, ProvinceInstance const* rhs) {
	return lhs->get_owner() == rhs->get_owner() && lhs->get_colony_status() == rhs->get_colony_status();
}

StateSet::StateSet(Region const& new_region) : region { new_region }, states {} {}

size_t StateSet::get_state_count() const {
	return states.size();
}

void StateSet::update_gamestate() {
	for (State& state : states) {
		state.update_gamestate();
	}
}

bool StateManager::add_state_set(MapInstance& map_instance, Region const& region) {
	if (region.get_meta()) {
		Logger::error("Cannot use meta region \"", region.get_identifier(), "\" as state template!");
		return false;
	}

	if (region.empty()) {
		Logger::error("Cannot use empty region \"", region.get_identifier(), "\" as state template!");
		return false;
	}

	std::vector<std::vector<ProvinceInstance*>> temp_provinces;

	for (ProvinceDefinition const* province : region.get_provinces()) {

		ProvinceInstance* province_instance = map_instance.get_province_instance_from_const(province);

		// add to existing state if shared owner & status...
		for (std::vector<ProvinceInstance*>& provinces : temp_provinces) {
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

	state_sets.push_back({ region });

	StateSet& state_set = state_sets.back();

	// Reserve space for the maximum number of states (one per province)
	state_set.states.reserve(region.size());

	for (std::vector<ProvinceInstance*>& provinces : temp_provinces) {
		ProvinceInstance* capital = provinces.front();

		state_set.states.push_back(
			/* TODO: capital province logic */
			{ state_set, capital->get_owner(), capital, std::move(provinces), capital->get_colony_status() }
		);

		State const& state = state_set.states.back();

		for (ProvinceInstance* province : state.get_provinces()) {
			province->set_state(&state);
		}
	}

	return true;
}

bool StateManager::generate_states(MapInstance& map_instance) {
	MapDefinition const& map_definition = map_instance.get_map_definition();

	state_sets.clear();
	state_sets.reserve(map_definition.get_region_count());

	bool ret = true;
	size_t state_count = 0;

	for (Region const& region : map_definition.get_regions()) {
		if (!region.get_meta()) {
			if (add_state_set(map_instance, region)) {
				state_count += state_sets.back().get_state_count();
			} else {
				ret = false;
			}
		}
	}

	Logger::info("Generated ", state_count, " states across ", state_sets.size(), " state sets.");

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
