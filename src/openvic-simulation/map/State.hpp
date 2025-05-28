#pragma once

#include <string>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/pop/PopType.hpp"
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
		CountryInstance* PROPERTY_PTR(owner);
		ProvinceInstance* PROPERTY_PTR(capital);
		std::vector<ProvinceInstance*> PROPERTY(provinces);
		ProvinceInstance::colony_status_t PROPERTY(colony_status);

		pop_size_t PROPERTY(total_population, 0);
		fixed_point_t PROPERTY(yesterdays_import_value);
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);

		IndexedMap<Strata, pop_size_t> PROPERTY(population_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(militancy_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(life_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(everyday_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(luxury_needs_fulfilled_by_strata);

		IndexedMap<PopType, pop_size_t> PROPERTY(pop_type_distribution);
		IndexedMap<PopType, pop_size_t> PROPERTY(pop_type_unemployed_count);
		IndexedMap<PopType, std::vector<Pop*>> PROPERTY(pops_cache_by_type);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Issue const*> PROPERTY(issue_distribution);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(vote_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);

		fixed_point_t PROPERTY(industrial_power);

		size_t PROPERTY(max_supported_regiments, 0);

		State(
			StateSet const& new_state_set,
			CountryInstance* new_owner,
			ProvinceInstance* new_capital,
			std::vector<ProvinceInstance*>&& new_provinces,
			ProvinceInstance::colony_status_t new_colony_status,
			decltype(population_by_strata)::keys_type const& strata_keys,
			decltype(pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(ideology_distribution)::keys_type const& ideology_keys
		);

	public:
		std::string get_identifier() const;

		constexpr bool is_colonial_state() const {
			return ProvinceInstance::is_colonial(colony_status);
		}

		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0
		constexpr pop_size_t get_pop_type_proportion(PopType const& pop_type) const {
			return pop_type_distribution[pop_type];
		}
		constexpr pop_size_t get_pop_type_unemployed(PopType const& pop_type) const {
			return pop_type_unemployed_count[pop_type];
		}
		constexpr fixed_point_t get_ideology_support(Ideology const& ideology) const {
			return ideology_distribution[ideology];
		}
		fixed_point_t get_issue_support(Issue const& issue) const;
		fixed_point_t get_party_support(CountryParty const& party) const;
		fixed_point_t get_culture_proportion(Culture const& culture) const;
		fixed_point_t get_religion_proportion(Religion const& religion) const;
		constexpr pop_size_t get_strata_population(Strata const& strata) const {
			return population_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_militancy(Strata const& strata) const {
			return militancy_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_life_needs_fulfilled(Strata const& strata) const {
			return life_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_everyday_needs_fulfilled(Strata const& strata) const {
			return everyday_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_luxury_needs_fulfilled(Strata const& strata) const {
			return luxury_needs_fulfilled_by_strata[strata];
		}

		void update_gamestate();
	};

	struct Region;

	struct StateSet {
		friend struct StateManager;

		using states_t = plf::colony<State>;

	private:
		Region const& PROPERTY(region);
		states_t PROPERTY(states);

	public:
		StateSet(Region const& new_region);

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
			decltype(State::population_by_strata)::keys_type const& strata_keys,
			decltype(State::pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(State::ideology_distribution)::keys_type const& ideology_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(
			MapInstance& map_instance,
			decltype(State::population_by_strata)::keys_type const& strata_keys,
			decltype(State::pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(State::ideology_distribution)::keys_type const& ideology_keys
		);

		void reset();

		void update_gamestate();
	};
}
