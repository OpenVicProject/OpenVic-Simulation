#pragma once

#include "openvic-simulation/misc/GameAdvancementHook.hpp"
#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/EconomyManager.hpp"
#include "openvic-simulation/history/HistoryManager.hpp"
#include "openvic-simulation/interface/UI.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/military/MilitaryManager.hpp"
#include "openvic-simulation/misc/Define.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/tech/Technology.hpp"

namespace OpenVic {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

	private:
		Map PROPERTY_REF(map);
		DefineManager PROPERTY_REF(define_manager);
		EconomyManager PROPERTY_REF(economy_manager);
		MilitaryManager PROPERTY_REF(military_manager);
		ModifierManager PROPERTY_REF(modifier_manager);
		PoliticsManager PROPERTY_REF(politics_manager);
		HistoryManager PROPERTY_REF(history_manager);
		TechnologyManager PROPERTY_REF(technology_manager);
		PopManager PROPERTY_REF(pop_manager);
		CountryManager PROPERTY_REF(country_manager);
		UIManager PROPERTY_REF(ui_manager);
		GameAdvancementHook PROPERTY_REF(clock);

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

		bool reset();
		bool load_bookmark(Bookmark const* new_bookmark);

		bool expand_building(Province::index_t province_index, std::string_view building_type_identifier);

		/* Hardcoded data for defining things for which parsing from files has
		 * not been implemented, currently mapmodes and building types.
		 */
		bool load_hardcoded_defines();
	};
}
