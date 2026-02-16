#include "GameAction.hpp"

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"

using namespace OpenVic;

bool GameActionManager::VariantVisitor::operator() (none_argument_t const& argument) const {
	return false;
}

// Core
bool GameActionManager::VariantVisitor::operator() (tick_argument_t const& argument) const {
	instance_manager.tick();
	return true;
}

bool GameActionManager::VariantVisitor::operator() (set_pause_argument_t const& new_is_paused) const {
	const bool old_pause = instance_manager.get_simulation_clock().is_paused();
	instance_manager.get_simulation_clock().set_paused(type_safe::get(new_is_paused));
	return old_pause != instance_manager.get_simulation_clock().is_paused();
}

bool GameActionManager::VariantVisitor::operator() (set_speed_argument_t const& new_speed) const {
	const SimulationClock::speed_t old_speed = instance_manager.get_simulation_clock().get_simulation_speed();
	instance_manager.get_simulation_clock().set_simulation_speed(type_safe::get(new_speed));
	return old_speed != instance_manager.get_simulation_clock().get_simulation_speed();
}

bool GameActionManager::VariantVisitor::operator() (set_ai_argument_t const& argument) const {
	const auto [country_index, new_is_ai] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_AI called with invalid country index: {}", country_index);
		return false;
	}

	const bool old_ai = country->is_ai();
	country->set_ai(new_is_ai);
	return old_ai != country->is_ai();
}

// Production
bool GameActionManager::VariantVisitor::operator() (expand_province_building_argument_t const& argument) const {
	const auto [province_index, province_building_index] = argument;
	ProvinceInstance* province = instance_manager.get_map_instance().get_province_instance_by_index(province_index);

	if (OV_unlikely(province == nullptr)) {
		spdlog::error_s("GAME_ACTION_EXPAND_PROVINCE_BUILDING called with invalid province index: {}", province_index);
		return false;
	}

	return province->expand_building(province_building_index);
}

// Budget
bool GameActionManager::VariantVisitor::operator() (set_strata_tax_argument_t const& argument) const {
	const auto [country_index, strata_index, tax_rate] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_STRATA_TAX called with invalid country index: {}", country_index);
		return false;
	}

	Strata const* strata = instance_manager.definition_manager.get_pop_manager().get_strata_by_index(strata_index);

	if (OV_unlikely(strata == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_STRATA_TAX called with invalid strata index: {}", strata_index);
		return false;
	}

	country->set_strata_tax_rate_slider_value(*strata, tax_rate);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_army_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_ARMY_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_army_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_navy_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_NAVY_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_navy_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_construction_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_CONSTRUCTION_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_construction_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_education_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_EDUCATION_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_education_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_administration_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_ADMINISTRATION_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_administration_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_social_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_SOCIAL_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_social_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_military_spending_argument_t const& argument) const {
	const auto [country_index, spending] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_MILITARY_SPENDING called with invalid country index: {}", country_index);
		return false;
	}

	country->set_military_spending_slider_value(spending);
	return false;
}

bool GameActionManager::VariantVisitor::operator() (set_tariff_rate_argument_t const& argument) const {
	const auto [country_index, tariff_rate] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_TARIFF_RATE called with invalid country index: {}", country_index);
		return false;
	}

	country->set_tariff_rate_slider_value(tariff_rate);
	return false;
}

// Technology
bool GameActionManager::VariantVisitor::operator() (start_research_argument_t const& argument) const {
	const auto [country_index, technology_index] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_START_RESEARCH called with invalid country index: {}", country_index);
		return false;
	}

	Technology const* technology = instance_manager.definition_manager
		.get_research_manager()
		.get_technology_manager()
		.get_technology_by_index(technology_index);

	if (OV_unlikely(technology == nullptr)) {
		spdlog::error_s("GAME_ACTION_START_RESEARCH called with invalid technology index: {}", technology_index);
		return false;
	}

	Technology const* old_research = country->get_current_research_untracked();

	country->start_research(*technology, instance_manager.get_today());

	return old_research != country->get_current_research_untracked();
}

// Politics

// Population

// Trade
bool GameActionManager::VariantVisitor::operator() (set_good_automated_argument_t const& argument) const {
	const auto [country_index, good_index, new_is_automated] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_GOOD_AUTOMATED called with invalid country index: {}", country_index);
		return false;
	}

	GoodInstance const* good = instance_manager.get_good_instance_manager().get_good_instance_by_index(good_index);

	if (OV_unlikely(good == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_GOOD_AUTOMATED called with invalid good index: {}", good_index);
		return false;
	}

	CountryInstance::good_data_t& good_data = country->get_good_data(*good);

	const bool old_automated = good_data.is_automated;

	good_data.is_automated = new_is_automated;

	return old_automated != good_data.is_automated;
}

bool GameActionManager::VariantVisitor::operator() (set_good_trade_order_argument_t const& argument) const {
	const auto [country_index, good_index, new_is_selling, new_cutoff] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_GOOD_TRADE_ORDER called with invalid country index: {}", country_index);
		return false;
	}

	GoodInstance const* good = instance_manager.get_good_instance_manager().get_good_instance_by_index(good_index);

	if (OV_unlikely(good == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_GOOD_TRADE_ORDER called with invalid good index: {}", good_index);
		return false;
	}

	CountryInstance::good_data_t& good_data = country->get_good_data(*good);

	if (OV_unlikely(good_data.is_automated)) {
		spdlog::error_s(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called for automated good! Country: {}, good: {}",
			*country, *good
		);
		return false;
	}

	const bool old_is_selling = good_data.is_selling;
	const fixed_point_t old_stockpile_cutoff = good_data.stockpile_cutoff;

	good_data.is_selling = new_is_selling;
	good_data.stockpile_cutoff = new_cutoff;

	if (good_data.stockpile_cutoff.is_negative()) {
		spdlog::error_s(
			"GAME_ACTION_SET_GOOD_TRADE_ORDER called with negative stockpile cutoff {} for {} good \"{}\" in country \"{}\". Setting to 0.",
			good_data.stockpile_cutoff,
			*good,
			good_data.is_selling ? "selling" : "buying",
			*country
		);
		good_data.stockpile_cutoff = 0;
	}

	return old_is_selling != good_data.is_selling || old_stockpile_cutoff != good_data.stockpile_cutoff;
}

// Diplomacy

// Military
bool GameActionManager::VariantVisitor::operator() (create_leader_argument_t const& argument) const {
	const auto [country_index, unit_branch] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_CREATE_LEADER called with invalid country index: {}", country_index);
		return false;
	}

	if (country->get_create_leader_count() < 1) {
		spdlog::error_s(
			"GAME_ACTION_CREATE_LEADER called for country \"{}\" without enough leadership points ({}) to create any leaders!",
			*country, country->get_leadership_point_stockpile().to_string(2)
		);
		return false;
	}

	return instance_manager.get_unit_instance_manager().create_leader(
		*country,
		unit_branch,
		instance_manager.get_today()
	);
}

bool GameActionManager::VariantVisitor::operator() (set_use_leader_argument_t const& argument) const {
	const auto [unique_id, new_should_use] = argument;

	LeaderInstance* leader = instance_manager.get_unit_instance_manager().get_leader_instance_by_unique_id(unique_id);

	if (OV_unlikely(leader == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_USE_LEADER called with invalid leader unique id: {}", unique_id);
		return false;
	}

	const bool old_use = leader->get_can_be_used();

	leader->set_can_be_used(new_should_use);

	return old_use != leader->get_can_be_used();
}

bool GameActionManager::VariantVisitor::operator() (set_auto_create_leaders_argument_t const& argument) const {
	const auto [country_index, new_should_auto_create] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_AUTO_CREATE_LEADERS called with invalid country index: {}", country_index);
		return false;
	}

	const bool old_auto_create = country->get_auto_create_leaders();

	country->set_auto_create_leaders(new_should_auto_create);

	return old_auto_create != country->get_auto_create_leaders();
}

bool GameActionManager::VariantVisitor::operator() (set_auto_assign_leaders_argument_t const& argument) const {
	const auto [country_index, new_should_auto_assign] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_AUTO_ASSIGN_LEADERS called with invalid country index: {}", country_index);
		return false;
	}

	const bool old_auto_assign = country->get_auto_assign_leaders();

	country->set_auto_assign_leaders(new_should_auto_assign);

	return old_auto_assign != country->get_auto_assign_leaders();
}

bool GameActionManager::VariantVisitor::operator() (set_mobilise_argument_t const& argument) const {
	const auto [country_index, new_is_mobilised] = argument;
	CountryInstance* country = instance_manager.get_country_instance_manager().get_country_instance_by_index(country_index);

	if (OV_unlikely(country == nullptr)) {
		spdlog::error_s("GAME_ACTION_SET_MOBILISE called with invalid country index: {}", country_index);
		return false;
	}

	const bool old_mobilise = country->is_mobilised();

	country->set_mobilised(new_is_mobilised);

	return old_mobilise != country->is_mobilised();
}

bool GameActionManager::VariantVisitor::operator() (start_land_unit_recruitment_argument_t const& argument) const {
	const auto [regiment_type_index, province_index, pop_id_in_province] = argument;

	RegimentType const* const regiment_type = instance_manager.definition_manager
		.get_military_manager()
		.get_unit_type_manager()
		.get_regiment_type_by_index(regiment_type_index);
	if (OV_unlikely(regiment_type == nullptr)) {
		spdlog::error_s("GAME_ACTION_START_LAND_UNIT_RECRUITMENT called with invalid regiment type index: {}", regiment_type_index);
		return false;
	}

	ProvinceInstance* province = instance_manager
		.get_map_instance()
		.get_province_instance_by_index(province_index);
	if (OV_unlikely(province == nullptr)) {
		spdlog::error_s("GAME_ACTION_START_LAND_UNIT_RECRUITMENT called with invalid province index: {}", province_index);
		return false;
	}

	Pop* pop = province->find_pop_by_id(pop_id_in_province);
	if (OV_unlikely(pop == nullptr)) {
		spdlog::error_s("GAME_ACTION_START_LAND_UNIT_RECRUITMENT called with invalid pop_id_in_province: {}", pop_id_in_province);
		return false;
	}

	//these TODO's should be implemented in ProvinceInstance and/or some military type
	//TODO verify pop's cultural status is acceptable for regiment type
	//TODO verify pop is recruitable and has enough size (pop.try_recruit())
	//TODO actually instantiate a regiment in recruitment state
	return false;
}
