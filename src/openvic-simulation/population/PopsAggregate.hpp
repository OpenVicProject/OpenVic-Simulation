#pragma once

#include <boost/int128/detail/int128_imp.hpp>

#include "openvic-simulation/population/PopSum.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/FixedVector.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct Ideology;
	struct PartyPolicy;
	struct Pop;
	struct PopsAggregateDeps;
	struct PopType;
	struct Reform;
	struct Religion;
	struct Strata;

	struct PopsAggregate {
	private:
		pop_sum_t PROPERTY(total_population, 0);
		size_t PROPERTY(max_supported_regiment_count, 0);
		fixed_point_t _yesterdays_import_value_running_total;
		OV_STATE_PROPERTY(fixed_point_t, yesterdays_import_value); //>= 0

		// TODO - population change (growth + migration), monthly totals + breakdown by source/destination
		boost::int128::int128_t literacy_running_total_raw;
		boost::int128::int128_t consciousness_running_total_raw;
		boost::int128::int128_t militancy_running_total_raw;
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);

		memory::FixedVector<boost::int128::int128_t, strata_index_t> militancy_by_strata_running_total_raw;
		memory::FixedVector<boost::int128::int128_t, strata_index_t> life_needs_fulfilled_by_strata_running_total_raw;
		memory::FixedVector<boost::int128::int128_t, strata_index_t> everyday_needs_fulfilled_by_strata_running_total_raw;
		memory::FixedVector<boost::int128::int128_t, strata_index_t> luxury_needs_fulfilled_by_strata_running_total_raw;
		memory::FixedVector<fixed_point_t, strata_index_t> SPAN_PROPERTY(militancy_by_strata);
		memory::FixedVector<fixed_point_t, strata_index_t> SPAN_PROPERTY(life_needs_fulfilled_by_strata);
		memory::FixedVector<fixed_point_t, strata_index_t> SPAN_PROPERTY(everyday_needs_fulfilled_by_strata);
		memory::FixedVector<fixed_point_t, strata_index_t> SPAN_PROPERTY(luxury_needs_fulfilled_by_strata);
		memory::FixedVector<pop_sum_t, strata_index_t> SPAN_PROPERTY(population_by_strata);

		memory::FixedVector<pop_sum_t, pop_type_index_t> SPAN_PROPERTY(population_by_type);
		memory::FixedVector<pop_sum_t, pop_type_index_t> SPAN_PROPERTY(unemployed_pops_by_type);
		memory::FixedVector<fixed_point_t, ideology_index_t> SPAN_PROPERTY(supporter_equivalents_by_ideology);
		memory::FixedVector<fixed_point_t, party_policy_index_t> SPAN_PROPERTY(supporter_equivalents_by_party_policy);
		memory::FixedVector<fixed_point_t, reform_index_t> SPAN_PROPERTY(supporter_equivalents_by_reform);
		fixed_point_map_t<CountryParty const*> PROPERTY(vote_equivalents_by_party);
		ordered_map<Culture const*, pop_sum_t> PROPERTY(population_by_culture);
		ordered_map<Religion const*, pop_sum_t> PROPERTY(population_by_religion);

	protected:
		PopsAggregate(PopsAggregateDeps const& deps);

		void clear_pops_aggregate();
		void add_pops_aggregate(PopsAggregate& part);
		void add_pops_aggregate(Pop const& pop);
		void normalise_pops_aggregate();
		void update_parties_for_votes(CountryDefinition const* country_definition);
		void update_parties_for_votes(CountryInstance const* country_instance);
	public:
		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0
		fixed_point_t get_vote_equivalents_by_party(CountryParty const& party) const;
		pop_sum_t get_population_by_culture(Culture const& culture) const;
		pop_sum_t get_population_by_religion(Religion const& religion) const;
	};
}
