#pragma once

#include <fmt/base.h>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/population/PopsAggregate.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct MapDefinition;
	struct Pop;
	struct PopsAggregateDeps;
	struct PopType;
	struct ProvinceInstance;
	struct Religion;
	struct Strata;

	struct MapInstance;

	/* Contains all current states.*/
	struct StateManager {
	private:
		memory::vector<StateSet> SPAN_PROPERTY(state_sets);

		bool add_state_set(
			MapInstance& map_instance, Region const& region,
			PopsAggregateDeps const& pops_aggregate_deps,
			forwardable_span<const Strata> strata_keys,
			forwardable_span<const PopType> pop_type_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(
			MapDefinition const& map_definition,
			MapInstance& map_instance,
			PopsAggregateDeps const& pops_aggregate_deps,
			forwardable_span<const Strata> strata_keys,
			forwardable_span<const PopType> pop_type_keys
		);

		void reset();

		void update_gamestate();
	};
}
