#include "State.hpp"

#include "Map.hpp"

using namespace OpenVic;

State::State(
	Country const* owner, Province const* capital, Region::provinces_t&& provinces, Province::colony_status_t colony_status
) : owner { owner }, capital { capital }, provinces { std::move(provinces) }, colony_status { colony_status } {}

StateSet::StateSet(Region const* new_region) {
	if (region->get_meta()) {
		Logger::error("Cannot use meta region as state template!");
	}
	region = new_region;

	std::vector<Region::provinces_t> temp_provinces;
	bool in_state = false;

	for (Province* province : region->get_provinces()) {
		// add to existing state if shared owner & status...
		for (Region::provinces_t& provinces : temp_provinces) {
			if (provinces[0] == province) {
				provinces.push_back(province);
				in_state = true;
				break;
			}
		}
		if (in_state) {
			in_state = false;
		} else {
			// ...otherwise start a new state
			temp_provinces.push_back({ province });
		}
	}

	for (Region::provinces_t& provinces : temp_provinces) {
		states.push_back({
			/* TODO: capital province logic */
			provinces[0]->get_owner(), provinces[0], std::move(provinces), provinces[0]->get_colony_status()
		});
	}

	// Go back and assign each new state to its provinces.
	for (State const& state : states) {
		for (Province* province : state.get_provinces()) {
			province->set_state(&state);
		}
	}
}

bool StateSet::add_state(State&& state) {
	const auto existing = std::find(states.begin(), states.end(), state);
	if (existing != states.end()) {
		Logger::error("Attempted to add existing state!");
		return false;
	}
	states.push_back(std::move(state));
	return true;
}

bool StateSet::remove_state(State const* state) {
	const auto existing = std::find(states.begin(), states.end(), *state);
	if (existing == states.end()) {
		Logger::error("Attempted to remove non-existant state!");
		return false;
	}
	states.erase(existing);
	return true;
}

StateSet::states_t& StateSet::get_states() {
	return states;
}

void StateManager::generate_states(Map const& map) {
	regions.clear();
	regions.reserve(map.get_region_count());
	for(Region const& region : map.get_regions()) {
		regions.push_back(StateSet(&region));
	}
	Logger::info("Generated states.");
}
