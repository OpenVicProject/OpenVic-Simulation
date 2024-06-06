#pragma once

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/diplomacy/DiplomacyManager.hpp"
#include "openvic-simulation/economy/EconomyManager.hpp"
#include "openvic-simulation/history/HistoryManager.hpp"
#include "openvic-simulation/interface/UI.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/military/MilitaryManager.hpp"
#include "openvic-simulation/misc/Decision.hpp"
#include "openvic-simulation/misc/Define.hpp"
#include "openvic-simulation/misc/Event.hpp"
#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/misc/SimulationClock.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/research/ResearchManager.hpp"
#include "openvic-simulation/scripts/ScriptManager.hpp"

namespace OpenVic {
	struct GameManager {
		using gamestate_updated_func_t = std::function<void()>;

	private:
		DefineManager PROPERTY_REF(define_manager);
		EconomyManager PROPERTY_REF(economy_manager);
		MilitaryManager PROPERTY_REF(military_manager);
		ModifierManager PROPERTY_REF(modifier_manager);
		PoliticsManager PROPERTY_REF(politics_manager);
		HistoryManager PROPERTY_REF(history_manager);
		ResearchManager PROPERTY_REF(research_manager);
		PopManager PROPERTY_REF(pop_manager);
		CountryManager PROPERTY_REF(country_manager);
		CountryInstanceManager PROPERTY_REF(country_instance_manager);
		CrimeManager PROPERTY_REF(crime_manager);
		EventManager PROPERTY_REF(event_manager);
		DecisionManager PROPERTY_REF(decision_manager);
		UIManager PROPERTY_REF(ui_manager);
		DiplomacyManager PROPERTY_REF(diplomacy_manager);
		/* Near the end so it is freed after other managers that may depend on it,
		 * e.g. if we want to remove military units from the province they're in when they're destructed. */
		Map PROPERTY_REF(map);
		MapmodeManager PROPERTY_REF(mapmode_manager);
		ScriptManager PROPERTY_REF(script_manager);
		SimulationClock PROPERTY_REF(simulation_clock);

		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark);
		Date PROPERTY(today);
		gamestate_updated_func_t gamestate_updated;
		bool gamestate_needs_update, currently_updating_gamestate;

		void set_gamestate_needs_update();
		void update_gamestate();
		void tick();

	public:
		GameManager(
			gamestate_updated_func_t gamestate_updated_callback,
			SimulationClock::state_changed_function_t clock_state_changed_callback
		);

		bool reset();
		bool load_bookmark(Bookmark const* new_bookmark);

		bool expand_selected_province_building(size_t building_index);
	};
}
