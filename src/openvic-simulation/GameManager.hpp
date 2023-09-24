#pragma once

#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/GameAdvancementHook.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/map/Map.hpp"
#include "openvic-simulation/units/Unit.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"

namespace OpenVic {
	struct GameManager {
		using state_updated_func_t = std::function<void()>;

	private:
		Map map;
		BuildingManager building_manager;
		GoodManager good_manager;
		PopManager pop_manager;
		IdeologyManager ideology_manager;
		IssueManager issue_manager;
		ProductionTypeManager production_type_manager;
		UnitManager unit_manager;
		ModifierManager modifier_manager;
		GameAdvancementHook clock;

		time_t session_start; /* SS-54, as well as allowing time-tracking */
		Date today;
		state_updated_func_t state_updated;
		bool needs_update;

		void set_needs_update();
		void update_state();
		void tick();

	public:
		GameManager(state_updated_func_t state_updated_callback);

		Map& get_map();
		Map const& get_map() const;
		BuildingManager& get_building_manager();
		BuildingManager const& get_building_manager() const;
		GoodManager& get_good_manager();
		GoodManager const& get_good_manager() const;
		PopManager& get_pop_manager();
		PopManager const& get_pop_manager() const;
		IdeologyManager& get_ideology_manager();
		IdeologyManager const& get_ideology_manager() const;
		IssueManager& get_issue_manager();
		IssueManager const& get_issue_manager() const;
		ProductionTypeManager& get_production_type_manager();
		ProductionTypeManager const& get_production_type_manager() const;
		UnitManager& get_unit_manager();
		UnitManager const& get_unit_manager() const;
		ModifierManager& get_modifier_manager();
		ModifierManager const& get_modifier_manager() const;
		GameAdvancementHook& get_clock();
		GameAdvancementHook const& get_clock() const;

		bool setup();

		Date const& get_today() const;
		bool expand_building(Province::index_t province_index, const std::string_view building_type_identifier);

		/* Hardcoded data for defining things for which parsing from files has
		 * not been implemented, currently mapmodes and building types.
		 */
		bool load_hardcoded_defines();
	};
}
