#pragma once

#include <plf_colony.h>

#include <fmt/base.h>

#include "openvic-simulation/population/PopsAggregate.hpp"
#include "openvic-simulation/types/ColonyStatus.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct Ideology;
	struct MapDefinition;
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
		CountryInstance* previous_country_ptr = nullptr;	

		ProvinceInstance* PROPERTY_PTR(capital);
		memory::vector<ProvinceInstance*> SPAN_PROPERTY(provinces);
		colony_status_t PROPERTY(colony_status);
		fixed_point_t PROPERTY(industrial_power);
		bool PROPERTY_CUSTOM_PREFIX(coastal, is, false);

		OV_IFLATMAP_PROPERTY(PopType, memory::vector<Pop*>, pops_cache_by_type);

		void _update_country();

	public:
		StateSet const& state_set;

		State(
			StateSet const& new_state_set,
			ProvinceInstance* new_capital,
			memory::vector<ProvinceInstance*>&& new_provinces,
			colony_status_t new_colony_status,
			forwardable_span<const Strata> strata_keys,
			forwardable_span<const PopType> pop_type_keys,
			forwardable_span<const Ideology> ideology_keys
		);
		State(State&&) = delete;
		State(State const&) = delete;
		State& operator=(State&&) = delete;
		State& operator=(State const&) = delete;

		memory::string get_identifier() const;
		CountryInstance* get_owner() const;

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
		states_t PROPERTY(states);

	public:
		Region const& region;

		StateSet(Region const& new_region);

		size_t get_state_count() const;

		void update_gamestate();
	};

	struct MapInstance;

	/* Contains all current states.*/
	struct StateManager {
	private:
		memory::vector<StateSet> SPAN_PROPERTY(state_sets);

		bool add_state_set(
			MapInstance& map_instance, Region const& region,
			forwardable_span<const Strata> strata_keys,
			forwardable_span<const PopType> pop_type_keys,
			forwardable_span<const Ideology> ideology_keys
		);

	public:
		/* Creates states from current province gamestate & regions, sets province state value.
		 * After this function, the `regions` property is unmanaged and must be carefully updated and
		 * validated by functions that modify it. */
		bool generate_states(
			MapDefinition const& map_definition,
			MapInstance& map_instance,
			forwardable_span<const Strata> strata_keys,
			forwardable_span<const PopType> pop_type_keys,
			forwardable_span<const Ideology> ideology_keys
		);

		void reset();

		void update_gamestate();
	};
}

template<>
struct fmt::formatter<OpenVic::State> : fmt::formatter<string_view> {
	format_context::iterator format(OpenVic::State const& state, fmt::format_context& ctx) const;
};
