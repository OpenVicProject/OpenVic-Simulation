#pragma once

#include "GameAdvancementHook.hpp"
#include "economy/Good.hpp"
#include "map/Map.hpp"

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

		return_t setup();

		Date const& get_today() const;
		return_t expand_building(Province::index_t province_index, const std::string_view building_type_identifier);
	};
}
