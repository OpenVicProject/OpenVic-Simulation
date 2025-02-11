#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace OpenVic {

	enum struct game_action_type_t : uint64_t {
		GAME_ACTION_NONE,

		// Core
		GAME_ACTION_TICK,
		GAME_ACTION_SET_PAUSE,
		GAME_ACTION_SET_SPEED,
		GAME_ACTION_SET_AI,

		// Production
		GAME_ACTION_EXPAND_PROVINCE_BUILDING,

		// Budget
		GAME_ACTION_SET_STRATA_TAX,
		GAME_ACTION_SET_LAND_SPENDING,
		GAME_ACTION_SET_NAVAL_SPENDING,
		GAME_ACTION_SET_CONSTRUCTION_SPENDING,
		GAME_ACTION_SET_EDUCATION_SPENDING,
		GAME_ACTION_SET_ADMINISTRATION_SPENDING,
		GAME_ACTION_SET_SOCIAL_SPENDING,
		GAME_ACTION_SET_MILITARY_SPENDING,
		GAME_ACTION_SET_TARIFF_RATE,

		// Technology
		GAME_ACTION_START_RESEARCH,

		// Politics

		// Population

		// Trade
		GAME_ACTION_SET_GOOD_AUTOMATED,
		// GAME_ACTION_SET_GOOD_BUY_SELL,
		// GAME_ACTION_SET_GOOD_STOCKPILE_CUTOFF,

		// Diplomacy

		// Military
		GAME_ACTION_CREATE_LEADER,
		GAME_ACTION_SET_USE_LEADER,
		GAME_ACTION_SET_AUTO_CREATE_LEADERS,
		GAME_ACTION_SET_AUTO_ASSIGN_LEADERS,
		GAME_ACTION_SET_MOBILISE,
		// rally point settings?

		MAX_GAME_ACTION
	};

	using game_action_argument_t = std::variant<
		std::monostate, bool, int64_t, std::pair<uint64_t, bool>, std::pair<uint64_t, uint64_t>,
		std::pair<uint64_t, int64_t>, std::tuple<uint64_t, uint64_t, int64_t>, std::tuple<uint64_t, uint64_t, bool>
	>;

	std::string game_action_argument_to_string(game_action_argument_t const& argument);

	using game_action_t = std::pair<game_action_type_t, game_action_argument_t>;

	struct InstanceManager;

	struct GameActionManager {
	private:
		InstanceManager& instance_manager;

	public:
		GameActionManager(InstanceManager& new_instance_manager);

		bool execute_game_action(game_action_t const& game_action);

	private:
		// Return value indicates whether a gamestate update is needed or not
		// TODO - replace return bool with value indicating what needs updating in the gamestate and/or UI?
		// Or send those signals via the InstanceManager&, e.g. if multiple values need to be sent for a single action
		using game_action_callback_t = bool(GameActionManager::*)(game_action_argument_t const&);

		static std::array<
			game_action_callback_t, static_cast<size_t>(game_action_type_t::MAX_GAME_ACTION)
		> GAME_ACTION_CALLBACKS;

		bool game_action_callback_none(game_action_argument_t const& argument);

		// Core
		bool game_action_callback_tick(game_action_argument_t const& argument);
		bool game_action_callback_set_pause(game_action_argument_t const& argument);
		bool game_action_callback_set_speed(game_action_argument_t const& argument);
		bool game_action_callback_set_ai(game_action_argument_t const& argument);

		// Production
		bool game_action_callback_expand_province_building(game_action_argument_t const& argument);

		// Budget
		bool game_action_callback_set_strata_tax(game_action_argument_t const& argument);
		bool game_action_callback_set_land_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_naval_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_construction_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_education_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_administration_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_social_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_military_spending(game_action_argument_t const& argument);
		bool game_action_callback_set_tariff_rate(game_action_argument_t const& argument);

		// Technology
		bool game_action_callback_start_research(game_action_argument_t const& argument);

		// Politics

		// Population

		// Trade
		bool game_action_callback_set_good_automated(game_action_argument_t const& argument);

		// Diplomacy

		// Military
		bool game_action_callback_create_leader(game_action_argument_t const& argument);
		bool game_action_callback_set_use_leader(game_action_argument_t const& argument);
		bool game_action_callback_set_auto_create_leaders(game_action_argument_t const& argument);
		bool game_action_callback_set_auto_assign_leaders(game_action_argument_t const& argument);
		bool game_action_callback_set_mobilise(game_action_argument_t const& argument);
	};
}
