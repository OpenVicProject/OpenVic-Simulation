#include "State.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;

State::State(
	StateSet const& new_state_set,
	ProvinceInstance* new_capital,
	memory::vector<std::reference_wrapper<ProvinceInstance>>&& new_provinces,
	colony_status_t new_colony_status,
	PopsAggregateDeps const& pops_aggregate_deps
) : PopsAggregate { pops_aggregate_deps },
	state_set { new_state_set },
	capital { new_capital },
	provinces { std::move(new_provinces) },
	colony_status { new_colony_status },
	pops_cache_by_type { generate_values, pops_aggregate_deps.pop_type_count }
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

void State::update_gamestate() {
	clear_pops_aggregate();

	for (memory::vector<std::reference_wrapper<Pop>>& pops_cache : pops_cache_by_type) {
		pops_cache.clear();
	}

	coastal = false;
	for (ProvinceInstance& province : provinces) {
		coastal |= province.province_definition.is_coastal();
		add_pops_aggregate(province);

		{
			pop_type_index_t pop_type_index {};
			for (auto const& province_pops_of_type : province.get_pops_cache_by_type()) {
				memory::vector<std::reference_wrapper<Pop>>& state_pops_of_type = pops_cache_by_type[pop_type_index];
				state_pops_of_type.insert(
					state_pops_of_type.end(),
					province_pops_of_type.begin(),
					province_pops_of_type.end()
				);
				++pop_type_index;
			}
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

// Whether two provinces in the same region should be grouped into the same state or not.
static bool provinces_belong_in_same_state(ProvinceInstance const& lhs, ProvinceInstance const& rhs) {
	return lhs.get_owner() == rhs.get_owner() && lhs.get_colony_status() == rhs.get_colony_status();
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

fmt::format_context::iterator fmt::formatter<State>::format(State const& state, fmt::format_context& ctx) const {
	return formatter<string_view>::format(state.get_identifier(), ctx);
}