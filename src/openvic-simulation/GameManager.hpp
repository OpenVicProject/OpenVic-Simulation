#pragma once

#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/map/Map.hpp"

namespace OpenVic {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

		Map map;
		BuildingManager building_manager;
		GoodManager good_manager;
		PopManager pop_manager;
		GameAdvancementHook clock;

	private:
		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Date today;
		state_updated_func_t state_updated;
		bool needs_update;

		void set_needs_update();
		void update_state();
		void tick();

	public:
		GameManager(state_updated_func_t state_updated_callback);
		Map* get_map();
		BuildingManager* get_building_manager();
		GoodManager* get_good_manager();
		PopManager* get_pop_manager();
		GameAdvancementHook* get_game_advancement_hook();

		bool setup();

		Date const& get_today() const;
		bool expand_building(Province::index_t province_index, const std::string_view building_type_identifier);

		/* Hardcoded data for defining things for which parsing from files has
		 * not been implemented, currently mapmodes and building types.
		 */
		bool load_hardcoded_defines();
	};
}
