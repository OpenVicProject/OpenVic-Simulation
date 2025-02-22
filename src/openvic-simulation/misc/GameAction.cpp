#include "GameAction.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"

using namespace OpenVic;

struct game_action_argument_print_visitor_t {
	std::ostream& stream;

	void operator()(std::monostate) {
		stream << "monostate";
	}

	void operator()(bool b) {
		stream << (b ? "true" : "false");
	}

	template<typename T1, typename T2>
	void operator()(std::pair<T1, T2> const& x) {
		stream << "(";
		(*this)(x.first);
		stream << ", ";
		(*this)(x.second);
		stream << ")";
	}

	template<size_t N = 0, typename... T>
	void operator()(std::tuple<T...> const& x) {
		if constexpr (N == 0) {
			stream << "(";
		}

		(*this)(std::get<N>(x));

		if constexpr (N + 1 < sizeof...(T)) {
			stream << ", ";
			(*this).operator()<N + 1, T...>(x);
		} else {
			stream << ")";
		}
	}

	void operator()(auto x) {
		stream << x;
	}
};

std::string OpenVic::game_action_argument_to_string(game_action_argument_t const& argument) {
	std::stringstream stream;

	std::visit(game_action_argument_print_visitor_t { stream }, argument);

	return stream.str();
}

GameActionManager::GameActionManager(InstanceManager& new_instance_manager)
	: instance_manager { new_instance_manager } {}

std::array<
	GameActionManager::game_action_callback_t, static_cast<size_t>(game_action_type_t::MAX_GAME_ACTION)
> GameActionManager::GAME_ACTION_CALLBACKS {
	// GAME_ACTION_NONE
	&GameActionManager::game_action_callback_none,

	// Core
	// GAME_ACTION_TICK
	&GameActionManager::game_action_callback_tick,
	// GAME_ACTION_SET_PAUSE
	&GameActionManager::game_action_callback_set_pause,
	// GAME_ACTION_SET_SPEED
	&GameActionManager::game_action_callback_set_speed,
	// GAME_ACTION_SET_AI
	&GameActionManager::game_action_callback_set_ai,

	// Production
	// GAME_ACTION_EXPAND_PROVINCE_BUILDING
	&GameActionManager::game_action_callback_expand_province_building,

	// Budget
	// GAME_ACTION_SET_STRATA_TAX
	&GameActionManager::game_action_callback_set_strata_tax,
	// GAME_ACTION_SET_LAND_SPENDING
	&GameActionManager::game_action_callback_set_land_spending,
	// GAME_ACTION_SET_NAVAL_SPENDING
	&GameActionManager::game_action_callback_set_naval_spending,
	// GAME_ACTION_SET_CONSTRUCTION_SPENDING
	&GameActionManager::game_action_callback_set_construction_spending,
	// GAME_ACTION_SET_EDUCATION_SPENDING
	&GameActionManager::game_action_callback_set_education_spending,
	// GAME_ACTION_SET_ADMINISTRATION_SPENDING
	&GameActionManager::game_action_callback_set_administration_spending,
	// GAME_ACTION_SET_SOCIAL_SPENDING
	&GameActionManager::game_action_callback_set_social_spending,
	// GAME_ACTION_SET_MILITARY_SPENDING
	&GameActionManager::game_action_callback_set_military_spending,
	// GAME_ACTION_SET_TARIFF_RATE
	&GameActionManager::game_action_callback_set_tariff_rate,

	// Technology
	// GAME_ACTION_START_RESEARCH
	&GameActionManager::game_action_callback_start_research,

	// Politics

	// Population

	// Trade
	// GAME_ACTION_SET_GOOD_AUTOMATED
	&GameActionManager::game_action_callback_set_good_automated,
	// GAME_ACTION_SET_GOOD_TRADE_ORDER
	&GameActionManager::game_action_callback_set_good_trade_order,

	// Diplomacy

	// Military
	// GAME_ACTION_CREATE_LEADER
	&GameActionManager::game_action_callback_create_leader,
	// GAME_ACTION_SET_USE_LEADER
	&GameActionManager::game_action_callback_set_use_leader,
	// GAME_ACTION_SET_AUTO_CREATE_LEADERS
	&GameActionManager::game_action_callback_set_auto_create_leaders,
	// GAME_ACTION_SET_AUTO_ASSIGN_LEADERS
	&GameActionManager::game_action_callback_set_auto_assign_leaders,
	// GAME_ACTION_SET_MOBILISE
	&GameActionManager::game_action_callback_set_mobilise
};

bool GameActionManager::execute_game_action(game_action_t const& game_action) {
	return (this->*GAME_ACTION_CALLBACKS[static_cast<size_t>(game_action.first)])(game_action.second);
}

bool GameActionManager::game_action_callback_none(game_action_argument_t const& argument) {
	if (OV_unlikely(!std::holds_alternative<std::monostate>(argument))) {
		Logger::warning("GAME_ACTION_NONE called with invalid argument: ", game_action_argument_to_string(argument));
	}

	return false;
}

// Core
bool GameActionManager::game_action_callback_tick(game_action_argument_t const& argument) {
	if (OV_unlikely(!std::holds_alternative<std::monostate>(argument))) {
		Logger::warning("GAME_ACTION_TICK called with invalid argument: ", game_action_argument_to_string(argument));
	}

	instance_manager.tick();
	return true;
}

bool GameActionManager::game_action_callback_set_pause(game_action_argument_t const& argument) {
	bool const* pause = std::get_if<bool>(&argument);
	if (OV_unlikely(pause == nullptr)) {
		Logger::error("GAME_ACTION_SET_PAUSE called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	const bool old_pause = instance_manager.get_simulation_clock().is_paused();

	instance_manager.get_simulation_clock().set_paused(*pause);

	return old_pause != instance_manager.get_simulation_clock().is_paused();
}

bool GameActionManager::game_action_callback_set_speed(game_action_argument_t const& argument) {
	int64_t const* speed = std::get_if<int64_t>(&argument);
	if (OV_unlikely(speed == nullptr)) {
		Logger::error("GAME_ACTION_SET_SPEED called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	const SimulationClock::speed_t old_speed = instance_manager.get_simulation_clock().get_simulation_speed();

	instance_manager.get_simulation_clock().set_simulation_speed(*speed);

	return old_speed != instance_manager.get_simulation_clock().get_simulation_speed();
}

bool GameActionManager::game_action_callback_set_ai(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_ai = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_ai == nullptr)) {
		Logger::error("GAME_ACTION_SET_AI called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_ai->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AI called with invalid country index: ", country_ai->first);
		return false;
	}

	const bool old_ai = country->is_ai();

	country->set_ai(country_ai->second);

	return old_ai != country->is_ai();
}

// Production
bool GameActionManager::game_action_callback_expand_province_building(game_action_argument_t const& argument) {
	std::pair<uint64_t, uint64_t> const* province_building = std::get_if<std::pair<uint64_t, uint64_t>>(&argument);
	if (OV_unlikely(province_building == nullptr)) {
		Logger::error(
			"GAME_ACTION_EXPAND_PROVINCE_BUILDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	ProvinceInstance* province = instance_manager.get_map_instance().get_province_instance_by_index(province_building->first);

	if (OV_unlikely(province == nullptr)) {
		Logger::error("GAME_ACTION_EXPAND_PROVINCE_BUILDING called with invalid province index: ", province_building->first);
		return false;
	}

	return province->expand_building(province_building->second);
}

// Budget
bool GameActionManager::game_action_callback_set_strata_tax(game_action_argument_t const& argument) {
	std::tuple<uint64_t, uint64_t, fixed_point_t> const* country_strata_value =
		std::get_if<std::tuple<uint64_t, uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_strata_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(std::get<0>(*country_strata_value));

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid country index: ", std::get<0>(*country_strata_value));
		return false;
	}

	Strata const* strata =
		instance_manager.get_definition_manager().get_pop_manager().get_strata_by_index(std::get<1>(*country_strata_value));

	if (OV_unlikely(strata == nullptr)) {
		Logger::error("GAME_ACTION_SET_STRATA_TAX called with invalid strata index: ", std::get<1>(*country_strata_value));
		return false;
	}

	SliderValue const& strata_tax_rate = country->get_tax_rate_slider_value_by_strata()[*strata];

	const fixed_point_t old_tax_rate = strata_tax_rate.get_value();

	country->set_strata_tax_rate_slider_value(*strata, std::get<2>(*country_strata_value));

	return old_tax_rate != strata_tax_rate.get_value();
}

bool GameActionManager::game_action_callback_set_land_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_LAND_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_LAND_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_land_spending = country->get_land_spending_slider_value().get_value();

	country->set_land_spending_slider_value(country_value->second);

	return old_land_spending != country->get_land_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_naval_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_NAVAL_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_NAVAL_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_naval_spending = country->get_naval_spending_slider_value().get_value();

	country->set_naval_spending_slider_value(country_value->second);

	return old_naval_spending != country->get_naval_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_construction_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_CONSTRUCTION_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_CONSTRUCTION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_construction_spending = country->get_construction_spending_slider_value().get_value();

	country->set_construction_spending_slider_value(country_value->second);

	return old_construction_spending != country->get_construction_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_education_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_EDUCATION_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_EDUCATION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_education_spending = country->get_education_spending_slider_value().get_value();

	country->set_education_spending_slider_value(country_value->second);

	return old_education_spending != country->get_education_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_administration_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_ADMINISTRATION_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_ADMINISTRATION_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_administration_spending = country->get_administration_spending_slider_value().get_value();

	country->set_administration_spending_slider_value(country_value->second);

	return old_administration_spending != country->get_administration_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_social_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_SOCIAL_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_SOCIAL_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_social_spending = country->get_social_spending_slider_value().get_value();

	country->set_social_spending_slider_value(country_value->second);

	return old_social_spending != country->get_social_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_military_spending(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_MILITARY_SPENDING called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_MILITARY_SPENDING called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_military_spending = country->get_military_spending_slider_value().get_value();

	country->set_military_spending_slider_value(country_value->second);

	return old_military_spending != country->get_military_spending_slider_value().get_value();
}

bool GameActionManager::game_action_callback_set_tariff_rate(game_action_argument_t const& argument) {
	std::pair<uint64_t, fixed_point_t> const* country_value = std::get_if<std::pair<uint64_t, fixed_point_t>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error("GAME_ACTION_SET_TARIFF_RATE called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_TARIFF_RATE called with invalid country index: ", country_value->first);
		return false;
	}

	const fixed_point_t old_tariff_rate = country->get_tariff_rate_slider_value().get_value();

	country->set_tariff_rate_slider_value(country_value->second);

	return old_tariff_rate != country->get_tariff_rate_slider_value().get_value();
}

// Technology
bool GameActionManager::game_action_callback_start_research(game_action_argument_t const& argument) {
	std::pair<uint64_t, uint64_t> const* country_tech = std::get_if<std::pair<uint64_t, uint64_t>>(&argument);
	if (OV_unlikely(country_tech == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_tech->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid country index: ", country_tech->first);
		return false;
	}

	Technology const* technology =
		instance_manager.get_definition_manager().get_research_manager().get_technology_manager().get_technology_by_index(
			country_tech->second
		);

	if (OV_unlikely(technology == nullptr)) {
		Logger::error("GAME_ACTION_START_RESEARCH called with invalid technology index: ", country_tech->second);
		return false;
	}

	Technology const* old_research = country->get_current_research();

	country->start_research(*technology, instance_manager);

	return old_research != country->get_current_research();
}

// Politics

// Population

// Trade
bool GameActionManager::game_action_callback_set_good_automated(game_action_argument_t const& argument) {
	std::tuple<uint64_t, uint64_t, bool> const* country_good_automated =
		std::get_if<std::tuple<uint64_t, uint64_t, bool>>(&argument);
	if (OV_unlikely(country_good_automated == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_AUTOMATED called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(std::get<0>(*country_good_automated));

	if (OV_unlikely(country == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_AUTOMATED called with invalid country index: ", std::get<0>(*country_good_automated)
		);
		return false;
	}

	GoodInstance const* good =
		instance_manager.get_good_instance_manager().get_good_instance_by_index(std::get<1>(*country_good_automated));

	if (OV_unlikely(good == nullptr)) {
		Logger::error("GAME_ACTION_SET_GOOD_AUTOMATED called with invalid good index: ", std::get<1>(*country_good_automated));
		return false;
	}

	CountryInstance::good_data_t& good_data = country->get_good_data(*good);

	const bool old_automated = good_data.is_automated;

	good_data.is_automated = std::get<2>(*country_good_automated);

	return old_automated != good_data.is_automated;
}

bool GameActionManager::game_action_callback_set_good_trade_order(game_action_argument_t const& argument) {
	std::tuple<uint64_t, uint64_t, bool, fixed_point_t> const* country_good_sell_amount =
		std::get_if<std::tuple<uint64_t, uint64_t, bool, fixed_point_t>>(&argument);
	if (OV_unlikely(country_good_sell_amount == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(std::get<0>(*country_good_sell_amount));

	if (OV_unlikely(country == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called with invalid country index: ", std::get<0>(*country_good_sell_amount)
		);
		return false;
	}

	GoodInstance const* good =
		instance_manager.get_good_instance_manager().get_good_instance_by_index(std::get<1>(*country_good_sell_amount));

	if (OV_unlikely(good == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called with invalid good index: ", std::get<1>(*country_good_sell_amount)
		);
		return false;
	}

	CountryInstance::good_data_t& good_data = country->get_good_data(*good);

	if (OV_unlikely(good_data.is_automated)) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called for automated good! Country: \"", country->get_identifier(),
			"\", good: \"", good->get_identifier(), "\", args: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	const bool old_is_selling = good_data.is_selling;
	const fixed_point_t old_stockpile_cutoff = good_data.stockpile_cutoff;

	good_data.is_selling = std::get<2>(*country_good_sell_amount);
	good_data.stockpile_cutoff = std::get<3>(*country_good_sell_amount);

	if (good_data.stockpile_cutoff.is_negative()) {
		Logger::error(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called with negative stockpile cutoff ", good_data.stockpile_cutoff,
			" for ", good_data.is_selling ? "selling" : "buying", " good \"", good->get_identifier(), "\" in country \"",
			country->get_identifier(), "\". Setting to 0."
		);
		good_data.stockpile_cutoff = fixed_point_t::_0();
	}

	return old_is_selling != good_data.is_selling || old_stockpile_cutoff != good_data.stockpile_cutoff;
}

// Diplomacy

// Military
bool GameActionManager::game_action_callback_create_leader(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_branch = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_branch == nullptr)) {
		Logger::error("GAME_ACTION_CREATE_LEADER called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_branch->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_CREATE_LEADER called with invalid country index: ", country_branch->first);
		return false;
	}

	if (country->get_create_leader_count() < 1) {
		Logger::error(
			"GAME_ACTION_CREATE_LEADER called for country \"", country->get_identifier(),
			"\" without enough leadership points (", country->get_leadership_point_stockpile().to_string(2),
			") to create any leaders!"
		);
		return false;
	}

	return instance_manager.get_unit_instance_manager().create_leader(
		*country,
		country_branch->second ? UnitType::branch_t::LAND : UnitType::branch_t::NAVAL,
		instance_manager.get_today()
	);
}

bool GameActionManager::game_action_callback_set_use_leader(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* leader_use = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(leader_use == nullptr)) {
		Logger::error("GAME_ACTION_SET_USE_LEADER called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	LeaderInstance* leader = instance_manager.get_unit_instance_manager().get_leader_instance_by_unique_id(leader_use->first);

	if (OV_unlikely(leader == nullptr)) {
		Logger::error("GAME_ACTION_SET_USE_LEADER called with invalid leader index: ", leader_use->first);
		return false;
	}

	const bool old_use = leader->get_can_be_used();

	leader->set_can_be_used(leader_use->second);

	return old_use != leader->get_can_be_used();
}

bool GameActionManager::game_action_callback_set_auto_create_leaders(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_value = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_AUTO_CREATE_LEADERS called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_CREATE_LEADERS called with invalid country index: ", country_value->first);
		return false;
	}

	const bool old_auto_create = country->get_auto_create_leaders();

	country->set_auto_create_leaders(country_value->second);

	return old_auto_create != country->get_auto_create_leaders();
}

bool GameActionManager::game_action_callback_set_auto_assign_leaders(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_value = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_value == nullptr)) {
		Logger::error(
			"GAME_ACTION_SET_AUTO_ASSIGN_LEADERS called with invalid argument: ", game_action_argument_to_string(argument)
		);
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_value->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_AUTO_ASSIGN_LEADERS called with invalid country index: ", country_value->first);
		return false;
	}

	const bool old_auto_assign = country->get_auto_assign_leaders();

	country->set_auto_assign_leaders(country_value->second);

	return old_auto_assign != country->get_auto_assign_leaders();
}

bool GameActionManager::game_action_callback_set_mobilise(game_action_argument_t const& argument) {
	std::pair<uint64_t, bool> const* country_mobilise = std::get_if<std::pair<uint64_t, bool>>(&argument);
	if (OV_unlikely(country_mobilise == nullptr)) {
		Logger::error("GAME_ACTION_SET_MOBILISE called with invalid argument: ", game_action_argument_to_string(argument));
		return false;
	}

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_index(country_mobilise->first);

	if (OV_unlikely(country == nullptr)) {
		Logger::error("GAME_ACTION_SET_MOBILISE called with invalid country index: ", country_mobilise->first);
		return false;
	}

	const bool old_mobilise = country->is_mobilised();

	country->set_mobilised(country_mobilise->second);

	return old_mobilise != country->is_mobilised();
}
