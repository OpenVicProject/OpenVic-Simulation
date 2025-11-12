#pragma once

#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/country/CountryInstanceManager.hpp"
#include "openvic-simulation/country/CountryInstanceDeps.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerDeps.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperationDeps.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/map/ProvinceInstanceDeps.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/misc/GameAction.hpp"
#include "openvic-simulation/misc/SimulationClock.hpp"
#include "openvic-simulation/politics/PoliticsInstanceManager.hpp"
#include "openvic-simulation/population/PopDeps.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"
#include "openvic-simulation/utility/Containers.hpp"

#include <function2/function2.hpp>

namespace OpenVic {

	struct DefinitionManager;
	struct Bookmark;

	struct InstanceManager {
		friend GameActionManager;

		using gamestate_updated_func_t = fu2::function_base<true, true, fu2::capacity_can_hold<void*>, false, false, void()>;

	private:
		ThreadPool thread_pool;

		CountryRelationManager PROPERTY_REF(country_relation_manager);
		DefinitionManager const& PROPERTY(definition_manager);
		const GameActionManager game_action_manager;
		GameRulesManager const& game_rules_manager;
		GoodInstanceManager PROPERTY_REF(good_instance_manager);
		MarketInstance PROPERTY_REF(market_instance);
		
		ArtisanalProducerDeps artisanal_producer_deps;
		CountryInstanceDeps country_instance_deps;
		PopDeps pop_deps;
		ResourceGatheringOperationDeps rgo_deps;
		ProvinceInstanceDeps province_instance_deps;

		FlagStrings PROPERTY_REF(global_flags);

		CountryInstanceManager PROPERTY_REF(country_instance_manager);
		UnitInstanceManager PROPERTY_REF(unit_instance_manager);
		PoliticsInstanceManager PROPERTY_REF(politics_instance_manager);
		/* Near the end so it is freed after other managers that may depend on it,
		 * e.g. if we want to remove military units from the province they're in when they're destructed. */
		MapInstance PROPERTY_REF(map_instance);
		SimulationClock PROPERTY_REF(simulation_clock);
		ConsoleInstance PROPERTY_REF(console_instance);

		bool PROPERTY_CUSTOM_PREFIX(game_instance_setup, is, false);
		bool PROPERTY_CUSTOM_PREFIX(game_session_started, is, false);

		time_t session_start = 0; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark, nullptr);
		Date PROPERTY(today);
		gamestate_updated_func_t gamestate_updated;
		memory::vector<game_action_t> game_action_queue;
		bool gamestate_needs_update = false, currently_updating_gamestate = false, currently_executing_game_actions = false;

		void update_modifier_sums();
		void set_gamestate_needs_update();
		void update_gamestate();
		void tick();

		void execute_game_actions();

	public:
		InstanceManager(
			GameRulesManager const& new_game_rules_manager,
			DefinitionManager const& new_definition_manager,
			gamestate_updated_func_t gamestate_updated_callback
		);

		inline constexpr bool is_bookmark_loaded() const {
			return bookmark != nullptr;
		}

		bool setup();
		bool load_bookmark(Bookmark const* new_bookmark);
		bool start_game_session();
		bool update_clock();
		void force_tick_and_update();

		bool set_today_and_update(Date new_today);

		bool queue_game_action(game_action_type_t type, game_action_argument_t&& argument);
	};
}
