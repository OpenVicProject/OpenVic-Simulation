#include "PopsAggregate.hpp"

#include <algorithm>
#include <cstdint>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"

using namespace OpenVic;
PopsAggregate::PopsAggregate(PopsAggregateDeps const& deps)
  : population_by_strata { generate_values, deps.strata_count },
	militancy_by_strata_running_total_raw { generate_values, deps.strata_count },
	life_needs_fulfilled_by_strata_running_total_raw { generate_values, deps.strata_count },
	everyday_needs_fulfilled_by_strata_running_total_raw { generate_values, deps.strata_count },
	luxury_needs_fulfilled_by_strata_running_total_raw { generate_values, deps.strata_count },
	militancy_by_strata { generate_values, deps.strata_count },
	life_needs_fulfilled_by_strata { generate_values, deps.strata_count },
	everyday_needs_fulfilled_by_strata { generate_values, deps.strata_count },
	luxury_needs_fulfilled_by_strata { generate_values, deps.strata_count },
	population_by_type { generate_values, deps.pop_type_count },
	unemployed_pops_by_type { generate_values, deps.pop_type_count },
	supporter_equivalents_by_ideology { generate_values, deps.ideology_count },
	supporter_equivalents_by_party_policy { generate_values, deps.party_policy_count },
	supporter_equivalents_by_reform { generate_values, deps.reform_count } {}

fixed_point_t PopsAggregate::get_vote_equivalents_by_party(CountryParty const& party) const {
	const decltype(vote_equivalents_by_party)::const_iterator it = vote_equivalents_by_party.find(&party);
	if (it == vote_equivalents_by_party.end()) {
		return 0;
	}
	return it.value();
}
pop_sum_t PopsAggregate::get_population_by_culture(Culture const& culture) const {
	const decltype(population_by_culture)::const_iterator it = population_by_culture.find(&culture);

	if (it != population_by_culture.end()) {
		return it->second;
	} else {
		return 0;
	}
}
pop_sum_t PopsAggregate::get_population_by_religion(Religion const& religion) const {
	const decltype(population_by_religion)::const_iterator it = population_by_religion.find(&religion);

	if (it != population_by_religion.end()) {
		return it->second;
	} else {
		return 0;
	}
}

template <typename... Vectors>
constexpr void bulk_fill_default(Vectors&... vecs) {
    (std::fill(vecs.begin(), vecs.end(), typename Vectors::value_type{}), ...);
}

void PopsAggregate::clear_pops_aggregate() {
	total_population = 0;
	max_supported_regiment_count = 0;
	_yesterdays_import_value_running_total = fixed_point_t::_0;

	literacy_running_total_raw = 0;
	consciousness_running_total_raw = 0;
	militancy_running_total_raw = 0;
	average_literacy = fixed_point_t::_0;
	average_consciousness = fixed_point_t::_0;
	average_militancy = fixed_point_t::_0;

	bulk_fill_default(
		militancy_by_strata_running_total_raw,
		life_needs_fulfilled_by_strata_running_total_raw,
		everyday_needs_fulfilled_by_strata_running_total_raw,
		luxury_needs_fulfilled_by_strata_running_total_raw,
		militancy_by_strata,
		life_needs_fulfilled_by_strata,
		everyday_needs_fulfilled_by_strata,
		luxury_needs_fulfilled_by_strata,
		population_by_strata,
		population_by_type,
		unemployed_pops_by_type,
		supporter_equivalents_by_ideology,
		supporter_equivalents_by_party_policy,
		supporter_equivalents_by_reform
	);

	vote_equivalents_by_party.clear();
	population_by_culture.clear();
	population_by_religion.clear();
}

constexpr void update_running_total_raw_128 (
	boost::int128::int128_t& running_total_raw,
	const pop_sum_t part_population,
	const fixed_point_t average
) {
	running_total_raw += type_safe::get(part_population) * static_cast<boost::int128::int128_t>(average.get_raw_value());
};

constexpr void add(std::span<pop_sum_t> total, std::span<const pop_sum_t> part) {
	std::transform(total.begin(), total.end(), part.begin(), total.begin(), std::plus<pop_sum_t>());
}
constexpr void add(std::span<fixed_point_t> total, std::span<const fixed_point_t> part) {
	std::transform(total.begin(), total.end(), part.begin(), total.begin(), std::plus<fixed_point_t>());
}

void PopsAggregate::add_pops_aggregate(PopsAggregate& part) {
	total_population += part.get_total_population();
	max_supported_regiment_count += part.get_max_supported_regiment_count();
	_yesterdays_import_value_running_total += part.get_yesterdays_import_value_untracked();

	const pop_sum_t part_population = part.get_total_population();
	update_running_total_raw_128(literacy_running_total_raw, part_population, part.get_average_literacy());
	update_running_total_raw_128(consciousness_running_total_raw, part_population, part.get_average_consciousness());
	update_running_total_raw_128(militancy_running_total_raw, part_population, part.get_average_militancy());

	{
		strata_index_t strata_index {};
		for (const pop_sum_t strata_population : part.get_population_by_strata()) {
			population_by_strata[strata_index] += strata_population;
			update_running_total_raw_128(
				militancy_by_strata_running_total_raw[strata_index],
				strata_population,
				part.militancy_by_strata[strata_index]
			);
			update_running_total_raw_128(
				life_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population,
				part.life_needs_fulfilled_by_strata[strata_index]
			);
			update_running_total_raw_128(
				everyday_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population,
				part.everyday_needs_fulfilled_by_strata[strata_index]
			);
			update_running_total_raw_128(
				luxury_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population,
				part.luxury_needs_fulfilled_by_strata[strata_index]
			);
			++strata_index;
		}
	}

	add(population_by_type, part.get_population_by_type());
	add(unemployed_pops_by_type, part.get_unemployed_pops_by_type());
	add(supporter_equivalents_by_ideology, part.get_supporter_equivalents_by_ideology());
	add(supporter_equivalents_by_party_policy, part.get_supporter_equivalents_by_party_policy());
	add(supporter_equivalents_by_reform, part.get_supporter_equivalents_by_reform());
	vote_equivalents_by_party += part.get_vote_equivalents_by_party();
	population_by_culture += part.get_population_by_culture();
	population_by_religion += part.get_population_by_religion();
}

void PopsAggregate::add_pops_aggregate(Pop const& pop) {
	const pop_size_t pop_size = pop.get_size();

	total_population += pop_size;
	yesterdays_import_value += pop.get_yesterdays_import_value().get_copy_of_value();
	const int64_t pop_size_v = type_safe::get(pop_size);
	update_running_total_raw_128(literacy_running_total_raw, pop_size, pop.get_literacy());
	update_running_total_raw_128(consciousness_running_total_raw, pop_size, pop.get_consciousness());
	update_running_total_raw_128(militancy_running_total_raw, pop_size, pop.get_militancy());

	PopType const& pop_type = pop.get_type();
	const pop_type_index_t pop_type_index = pop_type.index;
	const strata_index_t strata_index = pop_type.strata.index;

	population_by_strata[strata_index] += pop_size;
	update_running_total_raw_128(
		militancy_by_strata_running_total_raw[strata_index],
		pop_size, pop.get_militancy()
	);
	update_running_total_raw_128(
		life_needs_fulfilled_by_strata_running_total_raw[strata_index],
		pop_size, pop.get_life_needs_fulfilled()
	);
	update_running_total_raw_128(
		everyday_needs_fulfilled_by_strata_running_total_raw[strata_index],
		pop_size, pop.get_everyday_needs_fulfilled()
	);
	update_running_total_raw_128(
		luxury_needs_fulfilled_by_strata_running_total_raw[strata_index],
		pop_size, pop.get_luxury_needs_fulfilled()
	);

	population_by_type[pop_type_index] += pop_size;
	unemployed_pops_by_type[pop_type_index] += pop.get_unemployed();
	// Pop ideology, issue and vote distributions are scaled to pop size so we can add them directly
	add(supporter_equivalents_by_ideology, pop.get_supporter_equivalents_by_ideology());
	add(supporter_equivalents_by_party_policy, pop.get_supporter_equivalents_by_party_policy());
	add(supporter_equivalents_by_reform, pop.get_supporter_equivalents_by_reform());
	vote_equivalents_by_party += pop.get_vote_equivalents_by_party();
	population_by_culture[&pop.culture] += pop_size;
	population_by_religion[&pop.religion] += pop_size;

	if (pop_type.can_be_recruited) {
		max_supported_regiment_count += pop.get_max_supported_regiments();
	}
	yesterdays_import_value.set(_yesterdays_import_value_running_total);
}

constexpr void normalise(fixed_point_t& value, const boost::int128::int128_t running_total_raw, const pop_sum_t population) {
	value = fixed_point_t::parse_raw(static_cast<int64_t>(
		running_total_raw / type_safe::get(population)
	));
}

void PopsAggregate::normalise_pops_aggregate() {
	if (total_population > 0) {
		const int64_t total_population_v = type_safe::get(total_population);
		average_literacy = fixed_point_t::parse_raw(static_cast<int64_t>(literacy_running_total_raw / total_population_v));
		average_consciousness = fixed_point_t::parse_raw(static_cast<int64_t>(consciousness_running_total_raw / total_population_v));
		average_militancy = fixed_point_t::parse_raw(static_cast<int64_t>(militancy_running_total_raw / total_population_v));

		strata_index_t strata_index {};
		for (const pop_sum_t strata_population : population_by_strata) {
			normalise(
				militancy_by_strata[strata_index],
				militancy_by_strata_running_total_raw[strata_index],
				strata_population
			);
			normalise(
				life_needs_fulfilled_by_strata[strata_index],
				life_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population
			);
			normalise(
				everyday_needs_fulfilled_by_strata[strata_index],
				everyday_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population
			);
			normalise(
				luxury_needs_fulfilled_by_strata[strata_index],
				luxury_needs_fulfilled_by_strata_running_total_raw[strata_index],
				strata_population
			);
			++strata_index;
		}
	}
}

void PopsAggregate::update_parties_for_votes(CountryDefinition const* country_definition) {
	vote_equivalents_by_party.clear();
	if (country_definition == nullptr) {
		return;
	}

	auto view = country_definition->parties | std::views::transform(
		[](CountryParty const& key) {
			return std::make_pair(&key, fixed_point_t::_0);
		}
	);
	vote_equivalents_by_party.insert(view.begin(), view.end());
}
void PopsAggregate::update_parties_for_votes(CountryInstance const* country_instance) {
	if (country_instance == nullptr) {
		vote_equivalents_by_party.clear();
		return;
	}
	update_parties_for_votes(&country_instance->country_definition);
}
