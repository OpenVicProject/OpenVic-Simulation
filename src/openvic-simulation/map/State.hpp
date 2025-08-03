#pragma once

#include <plf_colony.h>

#include "openvic-simulation/pop/PopsAggregate.hpp"
#include "openvic-simulation/types/ColonyStatus.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
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

	struct State : PopsAggregate {
		friend struct StateManager;

	private:
		StateSet const& PROPERTY(state_set);
		CountryInstance* PROPERTY_PTR(owner);
		ProvinceInstance* PROPERTY_PTR(capital);
		memory::vector<ProvinceInstance*> SPAN_PROPERTY(provinces);
		colony_status_t PROPERTY(colony_status);
		fixed_point_t PROPERTY(industrial_power);

		IndexedFlatMap_PROPERTY(PopType, memory::vector<Pop*>, pops_cache_by_type);

	public:
		State(
			StateSet const& new_state_set,
			CountryInstance* new_owner,
			ProvinceInstance* new_capital,
			memory::vector<ProvinceInstance*>&& new_provinces,
			colony_status_t new_colony_status,
			utility::forwardable_span<const Strata> strata_keys,
			utility::forwardable_span<const PopType> pop_type_keys,
			utility::forwardable_span<const Ideology> ideology_keys
		);
		State(State&&) = delete;
		State(State const&) = delete;
		State& operator=(State&&) = delete;
		State& operator=(State const&) = delete;

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
			utility::forwardable_span<const Strata> strata_keys,
			utility::forwardable_span<const PopType> pop_type_keys,
			utility::forwardable_span<const Ideology> ideology_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(
			MapInstance& map_instance,
			utility::forwardable_span<const Strata> strata_keys,
			utility::forwardable_span<const PopType> pop_type_keys,
			utility::forwardable_span<const Ideology> ideology_keys
		);

		void reset();

		void update_gamestate();
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS