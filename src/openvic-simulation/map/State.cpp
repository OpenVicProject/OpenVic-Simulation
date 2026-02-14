#include "State.hpp"

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/scripts/Condition.hpp"

using namespace OpenVic;

State::State(
	StateSet const& new_state_set,
	ProvinceInstance* new_capital,
	memory::vector<ProvinceInstance*>&& new_provinces,
	colony_status_t new_colony_status,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<const PopType> pop_type_keys,
	forwardable_span<const Ideology> ideology_keys
) : PopsAggregate { strata_keys, pop_type_keys, ideology_keys },
	state_set { new_state_set },
	capital { new_capital },
	provinces { std::move(new_provinces) },
	colony_status { new_colony_status },
	pops_cache_by_type { pop_type_keys }
{
	_update_country();
}

memory::string State::get_identifier() const {
	return memory::fmt::format(
		"{}_{}_{}",
		state_set.region,
		ovfmt::validate(get_owner(), "NoCountry"),
		ProvinceInstance::get_colony_status_string(colony_status)
	);
}

CountryInstance* State::get_owner() const {
	return capital == nullptr
		? static_cast<CountryInstance*>(nullptr)
		: capital->get_owner();
}

memory::vector<Pop*> const& State::get_pops_cache_by_type(PopType const& pop_type) const {
	return pops_cache_by_type.at(pop_type);
}

void State::update_gamestate() {
	clear_pops_aggregate();

	for (memory::vector<Pop*>& pops_cache : pops_cache_by_type.get_values()) {
		pops_cache.clear();
	}

	coastal = false;
	for (ProvinceInstance* const province : provinces) {
		coastal |= province->province_definition.is_coastal();
		add_pops_aggregate(*province);

		for (auto const& [pop_type, province_pops_of_type] : province->get_pops_cache_by_type()) {
			memory::vector<Pop*>& state_pops_of_type = pops_cache_by_type.at(pop_type);
			state_pops_of_type.insert(
				state_pops_of_type.end(),
				province_pops_of_type.begin(),
				province_pops_of_type.end()
			);
		}
	}

	normalise_pops_aggregate();

	// TODO - use actual values when State has factory data
	const int32_t total_factory_levels_in_state = 0;
	const int32_t potential_workforce_in_state = 0; // sum of worker pops, regardless of employment
	const int32_t potential_employment_in_state = 0; // sum of (factory level * production method base_workforce_size)

	fixed_point_t workforce_scalar;
	constexpr fixed_point_t min_workforce_scalar = fixed_point_t::_0_20;
	constexpr fixed_point_t max_workforce_scalar = 4;
	if (potential_employment_in_state <= 0) {
		workforce_scalar = min_workforce_scalar;
	} else {
		workforce_scalar = std::clamp(
			(fixed_point_t { potential_workforce_in_state } / 100).floor() * 400 / potential_employment_in_state,
			min_workforce_scalar, max_workforce_scalar
		);
	}

	industrial_power = total_factory_levels_in_state * workforce_scalar;
	_update_country();
}

bool State::evaluate_leaf(ConditionNode const& node) const {
	std::string_view const& id = node.get_condition()->get_identifier();

	spdlog::warn_s("Condition {} not implemented in State::evaluate_leaf", id);
	return false;
}

void State::_update_country() {
	CountryInstance* const owner_ptr = get_owner();
	if (owner_ptr == previous_country_ptr) { 
		return;
	}
	
	update_parties_for_votes(owner_ptr);
	if (previous_country_ptr != nullptr) {
		previous_country_ptr->remove_state(*this);
	}

	if (owner_ptr != nullptr) {
		owner_ptr->add_state(*this);
	}

	previous_country_ptr = owner_ptr;
}

/* Whether two provinces in the same region should be grouped into the same state or not.
 * (Assumes both provinces non-null.) */
static bool provinces_belong_in_same_state(ProvinceInstance const* lhs, ProvinceInstance const* rhs) {
	return lhs->get_owner() == rhs->get_owner() && lhs->get_colony_status() == rhs->get_colony_status();
}

StateSet::StateSet(Region const& new_region) : region { new_region } {}

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
	forwardable_span<const Strata> strata_keys,
	forwardable_span<const PopType> pop_type_keys,
	forwardable_span<const Ideology> ideology_keys
) {
	OV_ERR_FAIL_COND_V_MSG(region.is_meta, false, memory::fmt::format("Cannot use meta region \"{}\" as state template!", region));
	OV_ERR_FAIL_COND_V_MSG(region.empty(), false, memory::fmt::format("Cannot use empty region \"{}\" as state template!", region));

	memory::vector<memory::vector<ProvinceInstance*>> temp_provinces;

	for (ProvinceDefinition const* province : region.get_provinces()) {

		ProvinceInstance* province_instance = &map_instance.get_province_instance_by_definition(*province);

		// add to existing state if shared owner & status...
		for (memory::vector<ProvinceInstance*>& provinces : temp_provinces) {
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

	state_sets.emplace_back(region);

	StateSet& state_set = state_sets.back();

	// Reserve space for the maximum number of states (one per province)
	state_set.states.reserve(region.size());

	for (memory::vector<ProvinceInstance*>& provinces : temp_provinces) {
		ProvinceInstance* capital = provinces.front();

		State& state = *state_set.states.emplace(
			/* TODO: capital province logic */
			state_set, capital,
			std::move(provinces), capital->get_colony_status(),
			strata_keys, pop_type_keys, ideology_keys
		);

		for (ProvinceInstance* province : state.get_provinces()) {
			province->set_state(&state);
		}
	}

	return true;
}

bool StateManager::generate_states(
	MapDefinition const& map_definition,
	MapInstance& map_instance,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<const PopType> pop_type_keys,
	forwardable_span<const Ideology> ideology_keys
) {
	state_sets.clear();
	state_sets.reserve(map_definition.get_region_count());

	bool ret = true;
	size_t state_count = 0;

	for (Region const& region : map_definition.get_regions()) {
		if (!region.is_meta) {
			if (add_state_set(map_instance, region, strata_keys, pop_type_keys, ideology_keys)) {
				state_count += state_sets.back().get_state_count();
			} else {
				ret = false;
			}
		}
	}

	SPDLOG_INFO("Generated {} states across {} state sets.", state_count, state_sets.size());

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

fmt::format_context::iterator fmt::formatter<State>::format(State const& state, fmt::format_context& ctx) const {
	return formatter<string_view>::format(state.get_identifier(), ctx);
}