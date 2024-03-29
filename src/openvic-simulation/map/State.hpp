#pragma once

#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/country/Country.hpp"

#include <deque>

namespace OpenVic {
	struct State {
	private:
		Country const* PROPERTY_RW(owner);
		Province const* PROPERTY_RW(capital);
		Region::provinces_t PROPERTY(provinces);
		Province::colony_status_t PROPERTY_RW(colony_status);

	public:
		State(
			Country const* owner, Province const* capital, Region::provinces_t&& provinces,
			Province::colony_status_t colony_status
		);
	};

	struct StateSet {
		using states_t = std::deque<State>;

	private:
		Region const& PROPERTY(region);
		states_t PROPERTY(states);

	public:
		StateSet(Map& map, Region const& new_region);

		states_t& get_states();
	};

	/* Contains all current states.*/
	struct StateManager {
	private:
		std::vector<StateSet> PROPERTY(regions);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		void generate_states(Map& map);
	};
} // namespace OpenVic
