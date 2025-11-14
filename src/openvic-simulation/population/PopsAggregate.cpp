#include "PopsAggregate.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"

using namespace OpenVic;
PopsAggregate::PopsAggregate(
	decltype(population_by_strata)::keys_span_type strata_keys,
	decltype(population_by_type)::keys_span_type pop_type_keys,
	decltype(supporter_equivalents_by_ideology)::keys_span_type ideology_keys
) : population_by_strata { strata_keys },
	militancy_by_strata { strata_keys },
	life_needs_fulfilled_by_strata { strata_keys },
	everyday_needs_fulfilled_by_strata { strata_keys },
	luxury_needs_fulfilled_by_strata { strata_keys },
	population_by_type { pop_type_keys },
	unemployed_pops_by_type { pop_type_keys },
	supporter_equivalents_by_ideology { ideology_keys } {}

pop_size_t PopsAggregate::get_population_by_type(PopType const& pop_type) const {
	return population_by_type.at(pop_type);
}
pop_size_t PopsAggregate::get_unemployed_pops_by_type(PopType const& pop_type) const {
	return unemployed_pops_by_type.at(pop_type);
}
fixed_point_t PopsAggregate::get_supporter_equivalents_by_ideology(Ideology const& ideology) const {
	return supporter_equivalents_by_ideology.at(ideology);
}
fixed_point_t PopsAggregate::get_supporter_equivalents_by_issue(BaseIssue const& issue) const {
	const decltype(supporter_equivalents_by_issue)::const_iterator it = supporter_equivalents_by_issue.find(&issue);

	if (it != supporter_equivalents_by_issue.end()) {
		return it->second;
	} else {
		return 0;
	}
}
fixed_point_t PopsAggregate::get_vote_equivalents_by_party(CountryParty const& party) const {
	const decltype(vote_equivalents_by_party)::const_iterator it = vote_equivalents_by_party.find(&party);
	if (it == vote_equivalents_by_party.end()) {
		return 0;
	}
	return it.value();
}
fixed_point_t PopsAggregate::get_population_by_culture(Culture const& culture) const {
	const decltype(population_by_culture)::const_iterator it = population_by_culture.find(&culture);

	if (it != population_by_culture.end()) {
		return it->second;
	} else {
		return 0;
	}
}
fixed_point_t PopsAggregate::get_population_by_religion(Religion const& religion) const {
	const decltype(population_by_religion)::const_iterator it = population_by_religion.find(&religion);

	if (it != population_by_religion.end()) {
		return it->second;
	} else {
		return 0;
	}
}
pop_size_t PopsAggregate::get_population_by_strata(Strata const& strata) const {
	return population_by_strata.at(strata);
}
fixed_point_t PopsAggregate::get_militancy_by_strata(Strata const& strata) const {
	return militancy_by_strata.at(strata);
}
fixed_point_t PopsAggregate::get_life_needs_fulfilled_by_strata(Strata const& strata) const {
	return life_needs_fulfilled_by_strata.at(strata);
}
fixed_point_t PopsAggregate::get_everyday_needs_fulfilled_by_strata(Strata const& strata) const {
	return everyday_needs_fulfilled_by_strata.at(strata);
}
fixed_point_t PopsAggregate::get_luxury_needs_fulfilled_by_strata(Strata const& strata) const {
	return luxury_needs_fulfilled_by_strata.at(strata);
}

void PopsAggregate::clear_pops_aggregate() {
	total_population = 0;
	max_supported_regiment_count = 0;
	_yesterdays_import_value_running_total = fixed_point_t::_0;

	average_literacy = fixed_point_t::_0;
	average_consciousness = fixed_point_t::_0;
	average_militancy = fixed_point_t::_0;

	population_by_strata.fill(0);
	militancy_by_strata.fill(fixed_point_t::_0);
	life_needs_fulfilled_by_strata.fill(fixed_point_t::_0);
	everyday_needs_fulfilled_by_strata.fill(fixed_point_t::_0);
	luxury_needs_fulfilled_by_strata.fill(fixed_point_t::_0);

	population_by_type.fill(0);
	unemployed_pops_by_type.fill(0);
	supporter_equivalents_by_ideology.fill(fixed_point_t::_0);
	supporter_equivalents_by_issue.clear();
	vote_equivalents_by_party.clear();
	population_by_culture.clear();
	population_by_religion.clear();
}

void PopsAggregate::add_pops_aggregate(PopsAggregate& part) {
	total_population += part.get_total_population();
	max_supported_regiment_count += part.get_max_supported_regiment_count();
	_yesterdays_import_value_running_total += part.get_yesterdays_import_value_untracked();

	// TODO - change casting if pop_size_t changes type
	const fixed_point_t part_population = fixed_point_t::parse(part.get_total_population());
	average_literacy += part.get_average_literacy() * part_population;
	average_consciousness += part.get_average_consciousness() * part_population;
	average_militancy += part.get_average_militancy() * part_population;

	population_by_strata += part.get_population_by_strata();
	militancy_by_strata.mul_add(part.get_militancy_by_strata(), part.get_population_by_strata());
	life_needs_fulfilled_by_strata.mul_add(
		part.get_life_needs_fulfilled_by_strata(), part.get_population_by_strata()
	);
	everyday_needs_fulfilled_by_strata.mul_add(
		part.get_everyday_needs_fulfilled_by_strata(), part.get_population_by_strata()
	);
	luxury_needs_fulfilled_by_strata.mul_add(
		part.get_luxury_needs_fulfilled_by_strata(), part.get_population_by_strata()
	);

	population_by_type += part.get_population_by_type();
	unemployed_pops_by_type += part.get_unemployed_pops_by_type();
	supporter_equivalents_by_ideology += part.get_supporter_equivalents_by_ideology();
	supporter_equivalents_by_issue += part.get_supporter_equivalents_by_issue();
	vote_equivalents_by_party += part.get_vote_equivalents_by_party();
	population_by_culture += part.get_population_by_culture();
	population_by_religion += part.get_population_by_religion();
}

void PopsAggregate::add_pops_aggregate(Pop const& pop) {
	const pop_size_t pop_size = pop.get_size();

	total_population += pop_size;
	yesterdays_import_value += pop.get_yesterdays_import_value().get_copy_of_value();
	average_literacy += pop.get_literacy() * pop_size;
	average_consciousness += pop.get_consciousness() * pop_size;
	average_militancy += pop.get_militancy() * pop_size;

	PopType const& pop_type = *pop.get_type();
	Strata const& strata = pop_type.strata;

	population_by_strata.at(strata) += pop_size;
	militancy_by_strata.at(strata) += pop.get_militancy() * pop_size;
	life_needs_fulfilled_by_strata.at(strata) += pop.get_life_needs_fulfilled() * pop_size;
	everyday_needs_fulfilled_by_strata.at(strata) += pop.get_everyday_needs_fulfilled() * pop_size;
	luxury_needs_fulfilled_by_strata.at(strata) += pop.get_luxury_needs_fulfilled() * pop_size;

	population_by_type.at(pop_type) += pop_size;
	unemployed_pops_by_type.at(pop_type) += pop.get_unemployed();
	// Pop ideology, issue and vote distributions are scaled to pop size so we can add them directly
	supporter_equivalents_by_ideology += pop.get_supporter_equivalents_by_ideology();
	supporter_equivalents_by_issue += pop.get_supporter_equivalents_by_issue();
	vote_equivalents_by_party += pop.get_vote_equivalents_by_party();
	population_by_culture[&pop.culture] += pop_size;
	population_by_religion[&pop.religion] += pop_size;

	max_supported_regiment_count += pop.get_max_supported_regiments();
	yesterdays_import_value.set(_yesterdays_import_value_running_total);
}

void PopsAggregate::normalise_pops_aggregate() {
	if (total_population > 0) {
		average_literacy /= total_population;
		average_consciousness /= total_population;
		average_militancy /= total_population;

		static const fu2::function<void(fixed_point_t&, pop_size_t const&)> handle_div_by_zero = [](
			fixed_point_t& lhs,
			pop_size_t const& rhs
		)->void {
			lhs = fixed_point_t::_0;
		};
		militancy_by_strata.divide_assign_handle_zero(
			population_by_strata,
			handle_div_by_zero
		);
		life_needs_fulfilled_by_strata.divide_assign_handle_zero(
			population_by_strata,
			handle_div_by_zero
		);
		everyday_needs_fulfilled_by_strata.divide_assign_handle_zero(
			population_by_strata,
			handle_div_by_zero
		);
		luxury_needs_fulfilled_by_strata.divide_assign_handle_zero(
			population_by_strata,
			handle_div_by_zero
		);
	}
}

void PopsAggregate::update_parties_for_votes(CountryDefinition const* country_definition) {
	vote_equivalents_by_party.clear();
	if (country_definition == nullptr) {
		return;
	}

	auto view = country_definition->get_parties() | std::views::transform(
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