#pragma once

#include <thread>

#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct MapDefinition;
	struct BuildingTypeManager;
	struct ProvinceHistoryManager;
	struct IssueManager;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct MapInstance {
	private:
		MapDefinition const& PROPERTY(map_definition);

		IdentifierRegistry<ProvinceInstance> IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(province_instance, 1);

		pop_size_t PROPERTY(highest_province_population, 0);
		pop_size_t PROPERTY(total_map_population, 0);

		StateManager PROPERTY_REF(state_manager);

		std::vector<PopValuesFromProvince> reusable_pop_values_collection;
		std::vector<std::thread> threads;

		void process_provinces_in_parallel(std::invocable<ProvinceInstance&, PopValuesFromProvince&> auto callback);

	public:
		MapInstance(MapDefinition const& new_map_definition);

		inline explicit constexpr operator MapDefinition const&() const {
			return map_definition;
		}

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(province_instance, 1);

		ProvinceInstance& get_province_instance_from_definition(ProvinceDefinition const& province);
		ProvinceInstance const& get_province_instance_from_definition(ProvinceDefinition const& province) const;

		bool setup(
			BuildingTypeManager const& building_type_manager,
			MarketInstance& market_instance,
			ModifierEffectCache const& modifier_effect_cache,
			PopsDefines const& pop_defines,
			decltype(ProvinceInstance::population_by_strata)::keys_type const& strata_keys,
			decltype(ProvinceInstance::pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(ProvinceInstance::ideology_distribution)::keys_type const& ideology_keys
		);
		bool apply_history_to_provinces(
			ProvinceHistoryManager const& history_manager,
			const Date date,
			CountryInstanceManager& country_manager,
			IssueManager const& issue_manager,
			MarketInstance& market_instance,
			ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
		);

		void update_modifier_sums(const Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(const Date today, DefineManager const& define_manager);
		void map_tick(const Date today);
		void initialise_for_new_game(const Date today, DefineManager const& define_manager);
	};
}
