#pragma once

#include <plf_colony.h>

#include "openvic-simulation/types/ColonyStatus.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct Ideology;
	struct Pop;
	struct PopType;
	struct ProvinceInstance;
	struct Religion;
	struct StateManager;
	struct StateSet;
	struct Strata;

	struct State {
		friend struct StateManager;

	private:
		StateSet const& PROPERTY(state_set);
		CountryInstance* PROPERTY_PTR(owner);
		ProvinceInstance* PROPERTY_PTR(capital);
		memory::vector<ProvinceInstance*> SPAN_PROPERTY(provinces);
		colony_status_t PROPERTY(colony_status);

		pop_size_t PROPERTY(total_population, 0);
		fixed_point_t PROPERTY(yesterdays_import_value);
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);

		IndexedFlatMap_PROPERTY(Strata, pop_size_t, population_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, militancy_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, life_needs_fulfilled_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, everyday_needs_fulfilled_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, luxury_needs_fulfilled_by_strata);

		IndexedFlatMap_PROPERTY(PopType, memory::vector<Pop*>, pops_cache_by_type);
		IndexedFlatMap_PROPERTY(PopType, pop_size_t, population_by_type);
		IndexedFlatMap_PROPERTY(PopType, pop_size_t, unemployed_pops_by_type);
		IndexedFlatMap_PROPERTY(Ideology, fixed_point_t, supporter_equivalents_by_ideology);
		fixed_point_map_t<BaseIssue const*> PROPERTY(supporter_equivalents_by_issue);
		fixed_point_map_t<CountryParty const*> PROPERTY(vote_equivalents_by_party);
		ordered_map<Culture const*, pop_size_t> PROPERTY(population_by_culture);
		ordered_map<Religion const*, pop_size_t> PROPERTY(population_by_religion);
	public:
		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0
		fixed_point_t get_supporter_equivalents_by_issue(Ideology const& ideology) const;
		fixed_point_t get_supporter_equivalents_by_issue(BaseIssue const& issue) const;
		fixed_point_t get_vote_equivalents_by_party(CountryParty const& party) const;
		fixed_point_t get_population_by_culture(Culture const& culture) const;
		fixed_point_t get_population_by_religion(Religion const& religion) const;
	private:
		fixed_point_t PROPERTY(industrial_power);

		size_t PROPERTY(max_supported_regiments, 0);

		State(
			StateSet const& new_state_set,
			CountryInstance* new_owner,
			ProvinceInstance* new_capital,
			memory::vector<ProvinceInstance*>&& new_provinces,
			colony_status_t new_colony_status,
			decltype(population_by_strata)::keys_span_type strata_keys,
			decltype(population_by_type)::keys_span_type pop_type_keys,
			decltype(supporter_equivalents_by_ideology)::keys_span_type ideology_keys
		);

	public:
		memory::string get_identifier() const;

		constexpr bool is_colonial_state() const {
			return is_colonial(colony_status);
		}

		void update_gamestate();
	};

	struct Region;

	struct StateSet {
		friend struct StateManager;

		using states_t = memory::colony<State>;

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
		memory::vector<StateSet> PROPERTY(state_sets);

		bool add_state_set(
			MapInstance& map_instance, Region const& region,
			decltype(State::population_by_strata)::keys_span_type strata_keys,
			decltype(State::population_by_type)::keys_span_type pop_type_keys,
			decltype(State::supporter_equivalents_by_ideology)::keys_span_type ideology_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(
			MapInstance& map_instance,
			decltype(State::population_by_strata)::keys_span_type strata_keys,
			decltype(State::population_by_type)::keys_span_type pop_type_keys,
			decltype(State::supporter_equivalents_by_ideology)::keys_span_type ideology_keys
		);

		void reset();

		void update_gamestate();
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS