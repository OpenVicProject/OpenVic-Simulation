#include "State.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

State::State(
	StateSet const& new_state_set,
	CountryInstance* new_owner,
	ProvinceInstance* new_capital,
	std::vector<ProvinceInstance*>&& new_provinces,
	ProvinceInstance::colony_status_t new_colony_status,
	decltype(pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(ideology_distribution)::keys_type const& ideology_keys
) : state_set { new_state_set },
	owner { new_owner },
	capital { new_capital },
	provinces { std::move(new_provinces) },
	colony_status { new_colony_status },
	pop_type_distribution { &pop_type_keys },
	pops_cache_by_type { &pop_type_keys },
	ideology_distribution { &ideology_keys },
	issue_distribution {},
	vote_distribution { new_owner != nullptr ? &new_owner->get_country_definition()->get_parties() : nullptr },
	culture_distribution {},
	religion_distribution {},
	industrial_power { 0 },
	max_supported_regiments { 0 }
	{}

std::string State::get_identifier() const {
	return StringUtils::append_string_views(
		state_set.get_region().get_identifier(), "_", owner->get_identifier(), "_",
		ProvinceInstance::get_colony_status_string(colony_status)
	);
}

fixed_point_t State::get_pop_type_proportion(PopType const& pop_type) const {
	return pop_type_distribution[pop_type];
}

fixed_point_t State::get_ideology_support(Ideology const& ideology) const {
	return ideology_distribution[ideology];
}

fixed_point_t State::get_issue_support(Issue const& issue) const {
	const decltype(issue_distribution)::const_iterator it = issue_distribution.find(&issue);

	if (it != issue_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t State::get_party_support(CountryParty const& party) const {
	if (vote_distribution.has_keys()) {
		return vote_distribution[party];
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t State::get_culture_proportion(Culture const& culture) const {
	const decltype(culture_distribution)::const_iterator it = culture_distribution.find(&culture);

	if (it != culture_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t State::get_religion_proportion(Religion const& religion) const {
	const decltype(religion_distribution)::const_iterator it = religion_distribution.find(&religion);

	if (it != religion_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

void State::update_gamestate() {
	total_population = 0;
	average_literacy = 0;
	average_consciousness = 0;
	average_militancy = 0;

	pop_type_distribution.clear();
	ideology_distribution.clear();
	issue_distribution.clear();
	vote_distribution.clear();
	culture_distribution.clear();
	religion_distribution.clear();

	for (std::vector<Pop*>& pops_cache : pops_cache_by_type.get_values()) {
		pops_cache.clear();
	}

	max_supported_regiments = 0;

	for (ProvinceInstance const* const province : provinces) {
		total_population += province->get_total_population();

		// TODO - change casting if pop_size_t changes type
		const fixed_point_t province_population = fixed_point_t::parse(province->get_total_population());
		average_literacy += province->get_average_literacy() * province_population;
		average_consciousness += province->get_average_consciousness() * province_population;
		average_militancy += province->get_average_militancy() * province_population;

		pop_type_distribution += province->get_pop_type_distribution();
		ideology_distribution += province->get_ideology_distribution();
		issue_distribution += province->get_issue_distribution();
		vote_distribution += province->get_vote_distribution();
		culture_distribution += province->get_culture_distribution();
		religion_distribution += province->get_religion_distribution();

		for (auto const& [pop_type, province_pops_of_type] : province->get_pops_cache_by_type()) {
			std::vector<Pop*>& state_pops_of_type = pops_cache_by_type[pop_type];
			state_pops_of_type.insert(
				state_pops_of_type.end(),
				province_pops_of_type.begin(),
				province_pops_of_type.end()
			);
		}

		max_supported_regiments += province->get_max_supported_regiments();
	}

	if (total_population > 0) {
		average_literacy /= total_population;
		average_consciousness /= total_population;
		average_militancy /= total_population;
	}

	// TODO - use actual values when State has factory data
	const int32_t total_factory_levels_in_state = 0;
	const int32_t potential_workforce_in_state = 0; // sum of worker pops, regardless of employment
	const int32_t potential_employment_in_state = 0; // sum of (factory level * production method base_workforce_size)

	fixed_point_t workforce_scalar;
	constexpr fixed_point_t min_workforce_scalar = fixed_point_t::_0_20();
	constexpr fixed_point_t max_workforce_scalar = fixed_point_t::_4();
	if (potential_employment_in_state <= 0) {
		workforce_scalar = min_workforce_scalar;
	} else {
		workforce_scalar = std::clamp(
			(fixed_point_t { potential_workforce_in_state } / 100).floor() * 400 / potential_employment_in_state,
			min_workforce_scalar, max_workforce_scalar
		);
	}

	industrial_power = total_factory_levels_in_state * workforce_scalar;
}

/* Whether two provinces in the same region should be grouped into the same state or not.
 * (Assumes both provinces non-null.) */
static bool provinces_belong_in_same_state(ProvinceInstance const* lhs, ProvinceInstance const* rhs) {
	return lhs->get_owner() == rhs->get_owner() && lhs->get_colony_status() == rhs->get_colony_status();
}

StateSet::StateSet(Region const& new_region) : region { new_region }, states {} {}

size_t StateSet::get_state_count() const {
	return states.size();
}

void StateSet::update_gamestate() {
	for (State& state : states) {
		state.update_gamestate();
	}
}

bool StateManager::add_state_set(
	MapInstance& map_instance, Region const& region,
	decltype(State::pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(State::ideology_distribution)::keys_type const& ideology_keys
) {
	if (region.get_meta()) {
		Logger::error("Cannot use meta region \"", region.get_identifier(), "\" as state template!");
		return false;
	}

	if (region.empty()) {
		Logger::error("Cannot use empty region \"", region.get_identifier(), "\" as state template!");
		return false;
	}

	std::vector<std::vector<ProvinceInstance*>> temp_provinces;

	for (ProvinceDefinition const* province : region.get_provinces()) {

		ProvinceInstance* province_instance = &map_instance.get_province_instance_from_definition(*province);

		// add to existing state if shared owner & status...
		for (std::vector<ProvinceInstance*>& provinces : temp_provinces) {
			if (provinces_belong_in_same_state(provinces.front(), province_instance)) {
				provinces.push_back(province_instance);
				// jump to the end of the outer loop, skipping the new state code
				goto loop_end;
			}
		}

		// ...otherwise start a new state
		temp_provinces.push_back({ province_instance });

	loop_end:;
		/* Either the province was added to an existing state and the program jumped to here,
		 * or it was used to create a new state and the program arrived here normally. */
	}

	state_sets.push_back({ region });

	StateSet& state_set = state_sets.back();

	// Reserve space for the maximum number of states (one per province)
	state_set.states.reserve(region.size());

	for (std::vector<ProvinceInstance*>& provinces : temp_provinces) {
		ProvinceInstance* capital = provinces.front();

		CountryInstance* owner = capital->get_owner();

		State& state = *state_set.states.insert(
			/* TODO: capital province logic */
			{ state_set, owner, capital, std::move(provinces), capital->get_colony_status(), pop_type_keys, ideology_keys }
		);

		for (ProvinceInstance* province : state.get_provinces()) {
			province->set_state(&state);
		}

		if (owner != nullptr) {
			owner->add_state(state);
		}
	}

	return true;
}

bool StateManager::generate_states(
	MapInstance& map_instance,
	decltype(State::pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(State::ideology_distribution)::keys_type const& ideology_keys
) {
	MapDefinition const& map_definition = map_instance.get_map_definition();

	state_sets.clear();
	state_sets.reserve(map_definition.get_region_count());

	bool ret = true;
	size_t state_count = 0;

	for (Region const& region : map_definition.get_regions()) {
		if (!region.get_meta()) {
			if (add_state_set(map_instance, region, pop_type_keys, ideology_keys)) {
				state_count += state_sets.back().get_state_count();
			} else {
				ret = false;
			}
		}
	}

	Logger::info("Generated ", state_count, " states across ", state_sets.size(), " state sets.");

	return ret;
}

void StateManager::reset() {
	state_sets.clear();
}

void StateManager::update_gamestate() {
	for (StateSet& state_set : state_sets) {
		state_set.update_gamestate();
	}
}