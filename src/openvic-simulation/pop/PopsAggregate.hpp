#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/IndexedFlatMapMacro.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct CountryDefinition;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct Ideology;
	struct Pop;
	struct PopType;
	struct Religion;
	struct Strata;

	struct PopsAggregate {
	private:
		pop_size_t PROPERTY(total_population, 0);
		size_t PROPERTY(max_supported_regiment_count, 0);
		fixed_point_t PROPERTY(yesterdays_import_value); //>= 0

		// TODO - population change (growth + migration), monthly totals + breakdown by source/destination
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);

		IndexedFlatMap_PROPERTY(Strata, pop_size_t, population_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, militancy_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, life_needs_fulfilled_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, everyday_needs_fulfilled_by_strata);
		IndexedFlatMap_PROPERTY(Strata, fixed_point_t, luxury_needs_fulfilled_by_strata);

		IndexedFlatMap_PROPERTY(PopType, pop_size_t, population_by_type);
		IndexedFlatMap_PROPERTY(PopType, pop_size_t, unemployed_pops_by_type);
		IndexedFlatMap_PROPERTY(Ideology, fixed_point_t, supporter_equivalents_by_ideology);
		fixed_point_map_t<BaseIssue const*> PROPERTY(supporter_equivalents_by_issue);
		fixed_point_map_t<CountryParty const*> PROPERTY(vote_equivalents_by_party);
		ordered_map<Culture const*, pop_size_t> PROPERTY(population_by_culture);
		ordered_map<Religion const*, pop_size_t> PROPERTY(population_by_religion);

	protected:
		PopsAggregate(
			decltype(population_by_strata)::keys_span_type strata_keys,
			decltype(population_by_type)::keys_span_type pop_type_keys,
			decltype(supporter_equivalents_by_ideology)::keys_span_type ideology_keys
		);

		void clear_pops_aggregate();
		void add_pops_aggregate(PopsAggregate const& part);
		void add_pops_aggregate(Pop const& pop);
		void normalise_pops_aggregate();
		void update_parties_for_votes(CountryDefinition const* country_definition);
		void update_parties_for_votes(CountryInstance const* country_instance);
	public:
		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0
		fixed_point_t get_supporter_equivalents_by_issue(Ideology const& ideology) const;
		fixed_point_t get_supporter_equivalents_by_issue(BaseIssue const& issue) const;
		fixed_point_t get_vote_equivalents_by_party(CountryParty const& party) const;
		fixed_point_t get_population_by_culture(Culture const& culture) const;
		fixed_point_t get_population_by_religion(Religion const& religion) const;
	};
}
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY