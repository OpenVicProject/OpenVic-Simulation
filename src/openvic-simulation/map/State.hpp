#pragma once

#include <vector>

#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct StateManager;
	struct StateSet;
	struct Country;
	struct ProvinceInstance;

	struct State {
		friend struct StateManager;

	private:
		StateSet const& PROPERTY(state_set);
		Country const* PROPERTY(owner);
		ProvinceInstance* PROPERTY(capital);
		std::vector<ProvinceInstance*> PROPERTY(provinces);
		ProvinceInstance::colony_status_t PROPERTY(colony_status);

		Pop::pop_size_t PROPERTY(total_population);

		State(
			StateSet const& new_state_set, Country const* owner, ProvinceInstance* capital,
			std::vector<ProvinceInstance*>&& provinces, ProvinceInstance::colony_status_t colony_status
		);

	public:
		void update_gamestate();
	};

	struct Region;

	struct StateSet {
		friend struct StateManager;

		// TODO - use a container that supports adding and removing items without invalidating pointers
		using states_t = std::vector<State>;

	private:
		Region const& PROPERTY(region);
		states_t PROPERTY(states);

		StateSet(Region const& new_region);

	public:
		size_t get_state_count() const;

		void update_gamestate();
	};

	struct MapInstance;

	/* Contains all current states.*/
	struct StateManager {
	private:
		std::vector<StateSet> PROPERTY(state_sets);

		bool add_state_set(MapInstance& map_instance, Region const& region);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(MapInstance& map_instance);

		void reset();

		void update_gamestate();
	};
}
