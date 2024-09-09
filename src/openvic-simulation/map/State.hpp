#pragma once

#include <string>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct StateManager;
	struct StateSet;
	struct CountryInstance;
	struct ProvinceInstance;

	struct State {
		friend struct StateManager;

	private:
		StateSet const& PROPERTY(state_set);
		CountryInstance* PROPERTY(owner);
		ProvinceInstance* PROPERTY(capital);
		std::vector<ProvinceInstance*> PROPERTY(provinces);
		ProvinceInstance::colony_status_t PROPERTY(colony_status);

		Pop::pop_size_t PROPERTY(total_population);
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);
		IndexedMap<PopType, fixed_point_t> PROPERTY(pop_type_distribution);

		fixed_point_t PROPERTY(industrial_power);

		size_t PROPERTY(max_supported_regiments);

		State(
			StateSet const& new_state_set,
			CountryInstance* new_owner,
			ProvinceInstance* new_capital,
			std::vector<ProvinceInstance*>&& new_provinces,
			ProvinceInstance::colony_status_t new_colony_status,
			decltype(pop_type_distribution)::keys_t const& pop_type_keys
		);

	public:
		std::string get_identifier() const;

		void update_gamestate();
	};

	struct Region;

	struct StateSet {
		friend struct StateManager;

		using states_t = plf::colony<State>;

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

		bool add_state_set(
			MapInstance& map_instance, Region const& region,
			decltype(State::pop_type_distribution)::keys_t const& pop_type_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(MapInstance& map_instance, decltype(State::pop_type_distribution)::keys_t const& pop_type_keys);

		void reset();

		void update_gamestate();
	};
}
