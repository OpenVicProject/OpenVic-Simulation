#pragma once

#include <functional>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/misc/SimulationClock.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {
	struct DefinitionManager;
	struct Bookmark;

	struct InstanceManager {
		using gamestate_updated_func_t = std::function<void()>;

	private:
		DefinitionManager const& PROPERTY(definition_manager);

		CountryInstanceManager PROPERTY_REF(country_instance_manager);
		CountryRelationManager PROPERTY_REF(country_relation_manager);
		GoodInstanceManager PROPERTY_REF(good_instance_manager);
		MarketInstance PROPERTY_REF(market_instance);
		UnitInstanceManager PROPERTY_REF(unit_instance_manager);
		/* Near the end so it is freed after other managers that may depend on it,
		 * e.g. if we want to remove military units from the province they're in when they're destructed. */
		MapInstance PROPERTY_REF(map_instance);
		SimulationClock PROPERTY_REF(simulation_clock);

		bool PROPERTY_CUSTOM_PREFIX(game_instance_setup, is);
		bool PROPERTY_CUSTOM_PREFIX(game_session_started, is);

		void update_modifier_sums();
	public:
		inline constexpr bool is_bookmark_loaded() const {
			return bookmark != nullptr;
		}

	private:
		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark);
		Date PROPERTY(today);
		gamestate_updated_func_t gamestate_updated;
		bool gamestate_needs_update, currently_updating_gamestate;

		void set_gamestate_needs_update();
		void update_gamestate();
		void tick();

	public:
		InstanceManager(
			DefinitionManager const& new_definition_manager, gamestate_updated_func_t gamestate_updated_callback,
			SimulationClock::state_changed_function_t clock_state_changed_callback
		);

		bool setup();
		bool load_bookmark(Bookmark const* new_bookmark);
		bool start_game_session();
		bool update_clock();

		bool expand_selected_province_building(size_t building_index);
	};
}
