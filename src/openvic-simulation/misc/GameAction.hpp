#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

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
		// TODO - enable/disable subsidies, open, close factory + ways to do these for all factories
		//      - build, expand and destroy factory
		//      - change a factory's hiring priority
		//      - invest in a project
		//      - foreign investment?

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
		// TODO - take and repay loans

		// Technology
		GAME_ACTION_START_RESEARCH,

		// Politics
		// TODO - change ruling party, hold elections, pass reform, suppress movement, westernise, take decision, release nation

		// Population
		// TODO - maybe put "set national focus" under this category?

		// Trade
		GAME_ACTION_SET_GOOD_AUTOMATED,
		GAME_ACTION_SET_GOOD_TRADE_ORDER,

		// Diplomacy
		// TODO - diplomatic actions, set influence priority, add wargoal, intervene in war, cancel CB justification, crisis stuff

		// Military
		GAME_ACTION_CREATE_LEADER,
		GAME_ACTION_SET_USE_LEADER,
		GAME_ACTION_SET_AUTO_CREATE_LEADERS,
		GAME_ACTION_SET_AUTO_ASSIGN_LEADERS,
		GAME_ACTION_SET_MOBILISE,
		// TODO - rally point settings, build army/navy, stack actions (move, merge, split, delete (undermanned), (dis)embark,
		//        set hunt rebels, reorganise, change leader, etc)

		// TODO - Others:
		//  - select event option
		//  - set country and intersting or not (buttons upon right clicking on flag icons)
		//  - upgrade colony status
		//  - execute commands (or should the console just convert commands which affect gamestate to regular game actions?)

		MAX_GAME_ACTION
	};

	using game_action_argument_t = std::variant<
		std::monostate, bool, int64_t, std::pair<uint64_t, bool>, std::pair<uint64_t, uint64_t>,
		std::pair<uint64_t, fixed_point_t>, std::tuple<uint64_t, uint64_t, fixed_point_t>,
		std::tuple<uint64_t, uint64_t, bool>, std::tuple<uint64_t, uint64_t, bool, fixed_point_t>
	>;

	std::string game_action_argument_to_string(game_action_argument_t const& argument);

	using game_action_t = std::pair<game_action_type_t, game_action_argument_t>;

	struct InstanceManager;

	struct GameActionManager {
	private:
		InstanceManager& instance_manager;

	public:
		GameActionManager(InstanceManager& new_instance_manager);

		bool execute_game_action(game_action_t const& game_action) const;

	private:
		// Return value indicates whether a gamestate update is needed or not
		// TODO - replace return bool with value indicating what needs updating in the gamestate and/or UI?
		// Or send those signals via the InstanceManager&, e.g. if multiple values need to be sent for a single action
		using game_action_callback_t = bool(GameActionManager::*)(game_action_argument_t const&) const;

		static std::array<
			game_action_callback_t, static_cast<size_t>(game_action_type_t::MAX_GAME_ACTION)
		> GAME_ACTION_CALLBACKS;

		bool game_action_callback_none(game_action_argument_t const& argument) const;

		// Core
		bool game_action_callback_tick(game_action_argument_t const& argument) const;
		bool game_action_callback_set_pause(game_action_argument_t const& argument) const;
		bool game_action_callback_set_speed(game_action_argument_t const& argument) const;
		bool game_action_callback_set_ai(game_action_argument_t const& argument) const;

		// Production
		bool game_action_callback_expand_province_building(game_action_argument_t const& argument) const;

		// Budget
		bool game_action_callback_set_strata_tax(game_action_argument_t const& argument) const;
		bool game_action_callback_set_land_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_naval_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_construction_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_education_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_administration_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_social_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_military_spending(game_action_argument_t const& argument) const;
		bool game_action_callback_set_tariff_rate(game_action_argument_t const& argument) const;

		// Technology
		bool game_action_callback_start_research(game_action_argument_t const& argument) const;

		// Politics

		// Population

		// Trade
		bool game_action_callback_set_good_automated(game_action_argument_t const& argument) const;
		bool game_action_callback_set_good_trade_order(game_action_argument_t const& argument) const;

		// Diplomacy

		// Military
		bool game_action_callback_create_leader(game_action_argument_t const& argument) const;
		bool game_action_callback_set_use_leader(game_action_argument_t const& argument) const;
		bool game_action_callback_set_auto_create_leaders(game_action_argument_t const& argument) const;
		bool game_action_callback_set_auto_assign_leaders(game_action_argument_t const& argument) const;
		bool game_action_callback_set_mobilise(game_action_argument_t const& argument) const;
	};
}
