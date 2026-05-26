#pragma once

#include <functional>
#include <string_view>

#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefines;
	struct CountryDefinitionManager;
	struct CountryInstanceDeps;
	struct CountryHistoryManager;
	struct InstanceManager;
	struct SharedCountryValuesDeps;
	struct ThreadPool;

	struct CountryInstanceManager {
	private:
		CountryDefinitionManager const& country_definition_manager;
		CountryDefines const& country_defines;
		SharedCountryValues shared_country_values;
		ThreadPool& thread_pool;

		memory::FixedVector<CountryInstance, country_index_t> SPAN_PROPERTY(country_instances);

		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(great_powers);
		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(secondary_powers);

		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(total_ranking);
		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(prestige_ranking);
		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(industrial_power_ranking);
		memory::vector<std::reference_wrapper<CountryInstance>> SPAN_PROPERTY(military_power_ranking);

		void update_rankings(const Date today);

	public:
		CountryInstanceManager(
			CountryDefines const& new_country_defines,
			CountryDefinitionManager const& new_country_definition_manager,
			CountryInstanceDeps const& country_instance_deps,
			SharedCountryValuesDeps const& shared_country_deps,
			ThreadPool& new_thread_pool
		);

		constexpr std::span<CountryInstance> get_country_instances() {
			return country_instances;
		}
		CountryInstance* get_country_instance_by_identifier(std::string_view identifier);
		CountryInstance const* get_country_instance_by_identifier(std::string_view identifier) const;
		constexpr CountryInstance& get_country_instance_by_index(const country_index_t index) {
			return country_instances[index];
		}
		constexpr CountryInstance const& get_country_instance_by_index(const country_index_t index) const {
			return country_instances[index];
		}
		CountryInstance& get_country_instance_by_definition(CountryDefinition const& country_definition);
		CountryInstance const& get_country_instance_by_definition(CountryDefinition const& country_definition) const;

		bool apply_history_to_countries(InstanceManager& instance_manager);

		void update_modifier_sums_before_map();
		void update_modifier_sums_after_map();
		void update_gamestate_after_map(const Date today);
		void country_manager_tick_before_map();
		void country_manager_tick_after_map();
	};
}
