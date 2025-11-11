#pragma once

#include <string_view>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefines;
	struct CountryDefinition;
	struct CountryDefinitionManager;
	struct CountryInstanceDeps;
	struct CountryHistoryManager;
	struct GoodInstanceManager;
	struct InstanceManager;
	struct MapInstance;
	struct PopsDefines;
	struct PopType;
	struct StaticModifierCache;
	struct ThreadPool;

	struct CountryInstanceManager {
	private:
		CountryDefinitionManager const& PROPERTY(country_definition_manager);
		CountryDefines const& country_defines;
		SharedCountryValues shared_country_values;
		ThreadPool& thread_pool;

		OV_IFLATMAP_PROPERTY(CountryDefinition, CountryInstance, country_instance_by_definition);

		memory::vector<CountryInstance*> SPAN_PROPERTY(great_powers);
		memory::vector<CountryInstance*> SPAN_PROPERTY(secondary_powers);

		memory::vector<CountryInstance*> SPAN_PROPERTY(total_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(prestige_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(industrial_power_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(military_power_ranking);

		void update_rankings(const Date today);

	public:
		CountryInstanceManager(
			CountryDefines const& new_country_defines,
			CountryDefinitionManager const& new_country_definition_manager,
			CountryInstanceDeps const& country_instance_deps,
			GoodInstanceManager const& new_good_instance_manager,
			PopsDefines const& new_pop_defines,
			utility::forwardable_span<const PopType> pop_type_keys,
			ThreadPool& new_thread_pool
		);

		constexpr std::span<CountryInstance> get_country_instances() {
			return country_instance_by_definition.get_values();
		}

		constexpr std::span<const CountryInstance> get_country_instances() const {
			return country_instance_by_definition.get_values();
		}
		CountryInstance* get_country_instance_by_identifier(std::string_view identifier);
		CountryInstance const* get_country_instance_by_identifier(std::string_view identifier) const;
		CountryInstance* get_country_instance_by_index(typename CountryInstance::index_t index);
		CountryInstance const* get_country_instance_by_index(typename CountryInstance::index_t index) const;
		CountryInstance& get_country_instance_by_definition(CountryDefinition const& country_definition); //const variant comes from OV_IFLATMAP_PROPERTY

		bool apply_history_to_countries(InstanceManager& instance_manager);

		void update_modifier_sums(const Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(const Date today, MapInstance& map_instance);
		void country_manager_tick_before_map();
		void country_manager_tick_after_map();
	};
}