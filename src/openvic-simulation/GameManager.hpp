#pragma once

#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/Modifier.hpp"
#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/EconomyManager.hpp"
#include "openvic-simulation/history/HistoryManager.hpp"
#include "openvic-simulation/interface/UI.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/military/MilitaryManager.hpp"
#include "openvic-simulation/misc/Define.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"

namespace OpenVic {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

	private:
		Map map;
		DefineManager define_manager;
		EconomyManager economy_manager;
		MilitaryManager military_manager;
		ModifierManager modifier_manager;
		PoliticsManager politics_manager;
		HistoryManager history_manager;
		PopManager pop_manager;
		CountryManager country_manager;
		UIManager ui_manager;
		GameAdvancementHook clock;

		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Bookmark const* PROPERTY(bookmark);
		Date PROPERTY(today);
		state_updated_func_t state_updated;
		bool needs_update;

		void set_needs_update();
		void update_state();
		void tick();

	public:
		GameManager(state_updated_func_t state_updated_callback);

		REF_GETTERS(map)
		REF_GETTERS(define_manager)
		REF_GETTERS(economy_manager)
		REF_GETTERS(military_manager)
		REF_GETTERS(modifier_manager)
		REF_GETTERS(politics_manager)
		REF_GETTERS(history_manager)
		REF_GETTERS(pop_manager)
		REF_GETTERS(country_manager)
		REF_GETTERS(ui_manager)
		REF_GETTERS(clock)

		bool reset();
		bool load_bookmark(Bookmark const* new_bookmark);

		bool expand_building(Province::index_t province_index, std::string_view building_type_identifier);

		/* Hardcoded data for defining things for which parsing from files has
		 * not been implemented, currently mapmodes and building types.
		 */
		bool load_hardcoded_defines();
	};
}
