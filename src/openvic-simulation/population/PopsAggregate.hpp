#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/population/PopSum.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

#include <boost/int128/detail/int128_imp.hpp>

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

		IndexedFlatMap<Strata, boost::int128::int128_t> militancy_by_strata_running_total_raw;
		IndexedFlatMap<Strata, boost::int128::int128_t> life_needs_fulfilled_by_strata_running_total_raw;
		IndexedFlatMap<Strata, boost::int128::int128_t> everyday_needs_fulfilled_by_strata_running_total_raw;
		IndexedFlatMap<Strata, boost::int128::int128_t> luxury_needs_fulfilled_by_strata_running_total_raw;
		OV_IFLATMAP_PROPERTY(Strata, fixed_point_t, militancy_by_strata);
		OV_IFLATMAP_PROPERTY(Strata, fixed_point_t, life_needs_fulfilled_by_strata);
		OV_IFLATMAP_PROPERTY(Strata, fixed_point_t, everyday_needs_fulfilled_by_strata);
		OV_IFLATMAP_PROPERTY(Strata, fixed_point_t, luxury_needs_fulfilled_by_strata);
		OV_IFLATMAP_PROPERTY(Strata, pop_sum_t, population_by_strata);

		OV_IFLATMAP_PROPERTY(PopType, pop_sum_t, population_by_type);
		OV_IFLATMAP_PROPERTY(PopType, pop_sum_t, unemployed_pops_by_type);
		OV_IFLATMAP_PROPERTY(Ideology, fixed_point_t, supporter_equivalents_by_ideology);
		OV_IFLATMAP_PROPERTY(PartyPolicy, fixed_point_t, supporter_equivalents_by_party_policy);
		OV_IFLATMAP_PROPERTY(Reform, fixed_point_t, supporter_equivalents_by_reform);
		fixed_point_map_t<CountryParty const*> PROPERTY(vote_equivalents_by_party);
		ordered_map<Culture const*, pop_sum_t> PROPERTY(population_by_culture);
		ordered_map<Religion const*, pop_sum_t> PROPERTY(population_by_religion);

	protected:
		PopsAggregate(
			PopsAggregateDeps const& pops_aggregate_deps,
			decltype(population_by_strata)::keys_span_type strata_keys,
			decltype(population_by_type)::keys_span_type pop_type_keys
		);

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