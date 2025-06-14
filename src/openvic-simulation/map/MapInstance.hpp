#pragma once

#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/pathfinding/AStarPathing.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct MapDefinition;
	struct BuildingTypeManager;
	struct ProvinceHistoryManager;
	struct IssueManager;
	struct ThreadPool;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct MapInstance {
		using canal_index_t = ProvinceDefinition::adjacency_t::data_t;

	private:
		MapDefinition const& PROPERTY(map_definition);
		ThreadPool& thread_pool;

		IdentifierRegistry<ProvinceInstance> IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(province_instance, 1);

		pop_size_t PROPERTY(highest_province_population, 0);
		pop_size_t PROPERTY(total_map_population, 0);

		StateManager PROPERTY_REF(state_manager);
		// TODO - should this be a vector of bools which we resize to the largest enabled canal index?
		ordered_set<canal_index_t> PROPERTY(enabled_canals);

		AStarPathing PROPERTY_REF(land_pathing);
		AStarPathing PROPERTY_REF(sea_pathing);

	public:
		MapInstance(
			MapDefinition const& new_map_definition,
			ThreadPool& new_thread_pool
		);

		inline explicit constexpr operator MapDefinition const&() const {
			return map_definition;
		}

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(province_instance, 1);

		ProvinceInstance& get_province_instance_from_definition(ProvinceDefinition const& province);
		ProvinceInstance const& get_province_instance_from_definition(ProvinceDefinition const& province) const;

		void enable_canal(canal_index_t canal_index);
		bool is_canal_enabled(canal_index_t canal_index) const;

		bool setup(
			BuildingTypeManager const& building_type_manager,
			MarketInstance& market_instance,
			GameRulesManager const& game_rules_manager,
			ModifierEffectCache const& modifier_effect_cache,
			decltype(ProvinceInstance::population_by_strata)::keys_span_type strata_keys,
			decltype(ProvinceInstance::pop_type_distribution)::keys_span_type pop_type_keys,
			decltype(ProvinceInstance::ideology_distribution)::keys_span_type ideology_keys
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
		void update_gamestate(InstanceManager const& instance_manager);
		void map_tick();
		void initialise_for_new_game(InstanceManager const& instance_manager);
	};
}
