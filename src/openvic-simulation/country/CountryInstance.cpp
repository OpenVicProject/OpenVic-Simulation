#include "CountryInstance.hpp"
#include "CountryInstanceDeps.hpp"
#include "CountryInstanceManager.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/defines/DiplomacyDefines.hpp"
#include "openvic-simulation/defines/EconomyDefines.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp" // IWYU pragma: keep for HasIndex requirement of copy_values_from
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/politics/Reform.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

using namespace OpenVic;

using enum CountryInstance::country_status_t;

static constexpr colour_t ERROR_COLOUR = colour_t::from_integer(0xFF0000);

CountryInstance::CountryInstance(
	CountryDefinition const* new_country_definition,
	SharedCountryValues* new_shared_country_values,
	CountryInstanceDeps const* country_instance_deps
) : CountryEconomy { *new_shared_country_values, *country_instance_deps },
	FlagStrings { "country" },
	HasIndex { new_country_definition->get_index() },
	PopsAggregate {
		country_instance_deps->stratas,
		country_instance_deps->pop_types,
		country_instance_deps->ideologies
	},
	/* Main attributes */
	country_definition { *new_country_definition },

	country_relations_manager { country_instance_deps->country_relations_manager },
	good_instance_manager { country_instance_deps->good_instance_manager },
	modifier_effect_cache { country_instance_deps->modifier_effect_cache },
	unit_type_manager { country_instance_deps->unit_type_manager },
	
	fallback_date_for_never_completing_research { country_instance_deps->fallback_date_for_never_completing_research },
	country_defines { country_instance_deps->country_defines },
	diplomacy_defines { country_instance_deps->diplomacy_defines },
	economy_defines { country_instance_deps->economy_defines },
	military_defines { country_instance_deps->military_defines },

	colour { ERROR_COLOUR },

	/* Production */
	building_type_unlock_levels { country_instance_deps->building_types },

	/* Technology */
	technology_unlock_levels { country_instance_deps->technologies },
	invention_unlock_levels { country_instance_deps->inventions },

	/* Politics */
	upper_house_proportion_by_ideology { country_instance_deps->ideologies },
	reforms { country_instance_deps->reforms },
	flag_overrides_by_government_type { country_instance_deps->government_types },
	crime_unlock_levels { country_instance_deps->crimes },

	/* Diplomacy */

	/* Military */
	regiment_type_unlock_levels { country_instance_deps->regiment_types },
	ship_type_unlock_levels { country_instance_deps->ship_types },

	/* DerivedState */
	desired_administrator_percentage{[this](DependencyTracker& tracker)->fixed_point_t {
		return country_defines.get_max_bureaucracy_percentage()
			+ total_administrative_multiplier.get(tracker) * country_defines.get_bureaucracy_percentage_increment();
	}},
	flag_government_type { [this](DependencyTracker& tracker)->GovernmentType const* {
		GovernmentType const* current_government_type = government_type.get(tracker);
		if (current_government_type == nullptr) {
			return nullptr;
		}
		GovernmentType const* flag_override = flag_overrides_by_government_type.at(*current_government_type);
		return flag_override == nullptr
			? current_government_type
			: flag_override;
	}},
	total_score{[this](DependencyTracker& tracker)->fixed_point_t {
		return prestige.get(tracker) + industrial_power.get(tracker) + military_power.get(tracker);
	}},
	military_power{[this](DependencyTracker& tracker)->fixed_point_t {
		return military_power_from_land.get(tracker) + military_power_from_sea.get(tracker) + military_power_from_leaders.get(tracker);
	}},
	research_progress{[this](DependencyTracker& tracker)->fixed_point_t {
		const fixed_point_t current_research_cost_copy = current_research_cost.get(tracker);
		return current_research_cost_copy > 0
			? invested_research_points.get(tracker) / current_research_cost_copy
			: fixed_point_t::_0;
	}} {
	modifier_sum.set_this_source(this);
	// Exclude PROVINCE (local) modifier effects from the country's modifier sum
	modifier_sum.set_this_excluded_targets(ModifierEffect::target_t::PROVINCE);

	update_parties_for_votes(new_country_definition);

	for (BuildingType const& building_type : building_type_unlock_levels.get_keys()) {
		if (building_type.is_default_enabled()) {
			unlock_building_type(building_type);
		}
	}

	for (Crime const& crime : crime_unlock_levels.get_keys()) {
		if (crime.is_default_active()) {
			unlock_crime(crime);
		}
	}

	for (RegimentType const& regiment_type : regiment_type_unlock_levels.get_keys()) {
		if (regiment_type.is_active()) {
			unlock_unit_type(regiment_type);
		}
	}

	for (ShipType const& ship_type : ship_type_unlock_levels.get_keys()) {
		if (ship_type.is_active()) {
			unlock_unit_type(ship_type);
		}
	}
}

std::string_view CountryInstance::get_identifier() const {
	return country_definition.get_identifier();
}

bool CountryInstance::exists() const {
	return !owned_provinces.empty();
}

bool CountryInstance::is_civilised() const {
	return country_status <= COUNTRY_STATUS_CIVILISED;
}

bool CountryInstance::can_colonise() const {
	return country_status <= COUNTRY_STATUS_SECONDARY_POWER;
}

bool CountryInstance::is_great_power() const {
	return country_status == COUNTRY_STATUS_GREAT_POWER;
}

bool CountryInstance::is_secondary_power() const {
	return country_status == COUNTRY_STATUS_SECONDARY_POWER;
}

bool CountryInstance::is_at_war() const {
	return !war_enemies.empty();
}

bool CountryInstance::is_neighbour(CountryInstance const& country) const {
	return neighbouring_countries.contains(&country);
}

CountryRelationManager::relation_value_type CountryInstance::get_relations_with(CountryInstance const& country) const {
	return country_relations_manager.get_country_relation(this, &country);
}

void CountryInstance::set_relations_with(CountryInstance& country, CountryRelationManager::relation_value_type relations) {
	country_relations_manager.set_country_relation(this, &country, relations);
}

bool CountryInstance::has_alliance_with(CountryInstance const& country) const {
	return country_relations_manager.get_country_alliance(this, &country);
}

void CountryInstance::set_alliance_with(CountryInstance& country, bool alliance) {
	country_relations_manager.set_country_alliance(this, &country, alliance);
}

bool CountryInstance::is_at_war_with(CountryInstance const& country) const {
	return war_enemies.contains(&country);
}

void CountryInstance::set_at_war_with(CountryInstance& country, bool at_war) {
	country_relations_manager.set_at_war_with(this, &country, at_war);
	if (at_war) {
		war_enemies.insert(&country);
		country.war_enemies.insert(this);
	} else {
		war_enemies.unordered_erase(&country);
		country.war_enemies.unordered_erase(this);
	}
}

bool CountryInstance::has_military_access_to(CountryInstance const& country) const {
	return country_relations_manager.get_has_military_access_to(this, &country);
}

void CountryInstance::set_military_access_to(CountryInstance& country, bool access) {
	country_relations_manager.set_has_military_access_to(this, &country, access);
}

bool CountryInstance::is_sending_war_subsidy_to(CountryInstance const& country) const {
	return country_relations_manager.get_war_subsidies_to(this, &country);
}

void CountryInstance::set_sending_war_subsidy_to(CountryInstance& country, bool sending) {
	country_relations_manager.set_war_subsidies_to(this, &country, sending);
}

bool CountryInstance::is_commanding_units(CountryInstance const& country) const {
	return country_relations_manager.get_commands_units(this, &country);
}

void CountryInstance::set_commanding_units(CountryInstance& country, bool commanding) {
	country_relations_manager.set_commands_units(this, &country, commanding);
}

bool CountryInstance::has_vision_of(CountryInstance const& country) const {
	return country_relations_manager.get_has_vision(this, &country);
}

void CountryInstance::set_has_vision_of(CountryInstance& country, bool vision) {
	country_relations_manager.set_has_vision(this, &country, vision);
}

CountryRelationManager::OpinionType CountryInstance::get_opinion_of(CountryInstance const& country) const {
	return country_relations_manager.get_country_opinion(this, &country);
}

void CountryInstance::set_opinion_of(CountryInstance& country, CountryRelationManager::OpinionType opinion) {
	if (OV_unlikely(country.sphere_owner.get_untracked() != nullptr && opinion == CountryRelationManager::OpinionType::Sphere)) {
		spdlog::warn_s("Attempting to add '{}' to a sphere when it is already included in a sphere.", country);
	}
	country_relations_manager.set_country_opinion(this, &country, opinion);
	if (opinion == CountryRelationManager::OpinionType::Sphere) {
		country.sphere_owner.set(this);
	} else if (country.sphere_owner.get_untracked() == this) {
		country.sphere_owner.set(nullptr);
	}
}

void CountryInstance::increase_opinion_of(CountryInstance& country) {
	CountryRelationManager::OpinionType opinion = country_relations_manager.get_country_opinion(this, &country);
	opinion++;
	set_opinion_of(country, opinion);
}

void CountryInstance::decrease_opinion_of(CountryInstance& country) {
	CountryRelationManager::OpinionType opinion = country_relations_manager.get_country_opinion(this, &country);
	opinion--;
	set_opinion_of(country, opinion);
}

CountryRelationManager::influence_value_type CountryInstance::get_influence_with(CountryInstance const& country) const {
	return country_relations_manager.get_influence_with(this, &country);
}

void CountryInstance::set_influence_with(CountryInstance& country, CountryRelationManager::influence_value_type influence) {
	country_relations_manager.set_influence_with(this, &country, influence);
}

CountryRelationManager::influence_priority_value_type CountryInstance::get_influence_priority_with(CountryInstance const& country) const {
	return country_relations_manager.get_influence_priority_with(this, &country);
}

void CountryInstance::set_influence_priority_with(CountryInstance& country, CountryRelationManager::influence_priority_value_type influence) {
	country_relations_manager.set_influence_priority_with(this, &country, influence);
}

std::optional<Date> CountryInstance::get_decredited_from_date(CountryInstance const& country) const {
	return country_relations_manager.get_discredited_date(this, &country);
}

void CountryInstance::set_discredited_from(CountryInstance& country, Date until) {
	country_relations_manager.set_discredited_date(this, &country, until);
}

std::optional<Date> CountryInstance::get_embass_banned_from_date(CountryInstance const& country) const {
	return country_relations_manager.get_embassy_banned_date(this, &country);
}

void CountryInstance::set_embassy_banned_from(CountryInstance& country, Date until) {
	country_relations_manager.set_embassy_banned_date(this, &country, until);
}

bool CountryInstance::can_army_units_enter(CountryInstance const& country) const {
	// TODO: include war allies, puppets
	return *this == country || is_at_war_with(country) || has_military_access_to(country);
}

bool CountryInstance::can_navy_units_enter(CountryInstance const& country) const {
	// TODO: include war allies, puppets
	return *this == country || has_military_access_to(country);
}

fixed_point_t CountryInstance::get_script_variable(memory::string const& variable_name) const {
	const decltype(script_variables)::const_iterator it = script_variables.find(variable_name);

	if (it != script_variables.end()) {
		return it->second;
	} else {
		return 0;
	}
}

void CountryInstance::set_script_variable(memory::string const& variable_name, fixed_point_t value) {
	script_variables[variable_name] = value;
}

void CountryInstance::change_script_variable(memory::string const& variable_name, fixed_point_t value) {
	script_variables[variable_name] += value;
}

bool CountryInstance::add_owned_province(ProvinceInstance& new_item) {
	OV_ERR_FAIL_COND_V_MSG(
		!owned_provinces.emplace(&new_item).second, false,
		memory::fmt::format("owned_province \"{}\" already present in country {}", new_item, *this)
	);
	return true;
}

bool CountryInstance::remove_owned_province(ProvinceInstance const& item_to_remove) {
	OV_ERR_FAIL_COND_V_MSG(
		owned_provinces.erase(&item_to_remove) == 0, false,
		memory::fmt::format("owned_province \"{}\" not present in country {}", item_to_remove, *this)
	);
	return true;
}

bool CountryInstance::add_controlled_province(ProvinceInstance& new_item) {
	OV_ERR_FAIL_COND_V_MSG(
		!controlled_provinces.emplace(&new_item).second, false,
		memory::fmt::format("controlled_province \"{}\" already present in country {}", new_item, *this)
	);
	return true;
}

bool CountryInstance::remove_controlled_province(ProvinceInstance const& item_to_remove) {
	OV_ERR_FAIL_COND_V_MSG(
		controlled_provinces.erase(&item_to_remove) == 0, false,
		memory::fmt::format("controlled_province \"{}\" not present in country {}", item_to_remove, *this)
	);
	return true;
}

bool CountryInstance::add_core_province(ProvinceInstance& new_item) {
	OV_ERR_FAIL_COND_V_MSG(
		!core_provinces.emplace(&new_item).second, false,
		memory::fmt::format("core_province \"{}\" already present in country {}", new_item, *this)
	);
	return true;
}

bool CountryInstance::remove_core_province(ProvinceInstance const& item_to_remove) {
	OV_ERR_FAIL_COND_V_MSG(
		core_provinces.erase(&item_to_remove) == 0, false,
		memory::fmt::format("core_province \"{}\" not present in country {}", item_to_remove, *this)
	);
	return true;
}

bool CountryInstance::add_state(State& new_item) {
	OV_ERR_FAIL_COND_V_MSG(
		!states.emplace(&new_item).second, false,
		memory::fmt::format("state \"{}\" already present in country {}", new_item, *this)
	);
	return true;
}

bool CountryInstance::remove_state(State const& item_to_remove) {
	OV_ERR_FAIL_COND_V_MSG(
		states.erase(&item_to_remove) == 0, false,
		memory::fmt::format("state \"{}\" not present in country {}", item_to_remove, *this)
	);
	return true;
}

bool CountryInstance::add_accepted_culture(Culture const& new_item) {
	OV_ERR_FAIL_COND_V_MSG(
		!accepted_cultures.emplace(&new_item).second, false,
		memory::fmt::format("accepted_culture \"{}\" already present in country {}", new_item, *this)
	);
	return true;
}

bool CountryInstance::remove_accepted_culture(Culture const& item_to_remove) {
	OV_ERR_FAIL_COND_V_MSG(
		accepted_cultures.erase(&item_to_remove) == 0, false,
		memory::fmt::format("accepted_culture \"{}\" not present in country {}", item_to_remove, *this)
	);
	return true;
}

bool CountryInstance::set_ruling_party(CountryParty const& new_ruling_party) {
	if (ruling_party.get_untracked() != &new_ruling_party) {
		ruling_party.set(&new_ruling_party);

		return update_rule_set();
	} else {
		return true;
	}
}

bool CountryInstance::add_reform(Reform const& new_reform) {
	ReformGroup const& reform_group = new_reform.get_reform_group();
	Reform const*& reform = reforms.at(reform_group);

	if (reform != &new_reform) {
		if (reform_group.get_is_administrative()) {
			if (reform != nullptr) {
				total_administrative_multiplier -= reform->get_administrative_multiplier();
			}
			total_administrative_multiplier += new_reform.get_administrative_multiplier();
		}

		reform = &new_reform;

		// TODO - if new_reform.get_reform_group().is_uncivilised() ?
		// TODO - new_reform.get_on_execute_trigger() / new_reform.get_on_execute_effect() ?

		return update_rule_set();
	} else {
		return true;
	}
}

void CountryInstance::change_war_exhaustion(fixed_point_t delta) {
	war_exhaustion = std::clamp(war_exhaustion + delta, fixed_point_t::_0, war_exhaustion_max);
}

bool CountryInstance::add_unit_instance_group(UnitInstanceGroup& group) {
	using enum unit_branch_t;

	switch (group.get_branch()) {
	case LAND:
		armies.push_back(static_cast<ArmyInstance*>(&group));
		return true;
	case NAVAL:
		navies.push_back(static_cast<NavyInstance*>(&group));
		return true;
	default:
		spdlog::error_s(
			"Trying to add unit group \"{}\" with invalid branch {} to country {}",
			group.get_name(), static_cast<uint32_t>(group.get_branch()), *this
		);
		return false;
	}
}

bool CountryInstance::remove_unit_instance_group(UnitInstanceGroup const& group) {
	const auto remove_from_vector = [this, &group]<unit_branch_t Branch>(
		memory::vector<UnitInstanceGroupBranched<Branch>*>& unit_instance_groups
	) -> bool {
		const typename memory::vector<UnitInstanceGroupBranched<Branch>*>::const_iterator it =
			std::find(unit_instance_groups.begin(), unit_instance_groups.end(), &group);

		if (it != unit_instance_groups.end()) {
			unit_instance_groups.erase(it);
			return true;
		} else {
			spdlog::error_s(
				"Trying to remove non-existent {} \"{}\" from country {}",
				get_branched_unit_group_name(Branch), group.get_name(), *this
			);
			return false;
		}
	};

	using enum unit_branch_t;

	switch (group.get_branch()) {
	case LAND:
		return remove_from_vector(armies);
	case NAVAL:
		return remove_from_vector(navies);
	default:
		spdlog::error_s(
			"Trying to remove unit group \"{}\" with invalid branch {} from country {}",
			group.get_name(), static_cast<uint32_t>(group.get_branch()), *this
		);
		return false;
	}
}

bool CountryInstance::add_leader(LeaderInstance& leader) {
	using enum unit_branch_t;

	switch (leader.get_branch()) {
	case LAND:
		generals.push_back(&leader);
		return true;
	case NAVAL:
		admirals.push_back(&leader);
		return true;
	default:
		spdlog::error_s(
			"Trying to add leader \"{}\" with invalid branch {} to country {}",
			leader.get_name(), static_cast<uint32_t>(leader.get_branch()), *this
		);
		return false;
	}
}

bool CountryInstance::remove_leader(LeaderInstance const& leader) {
	using enum unit_branch_t;

	memory::vector<LeaderInstance*>* leaders;

	switch (leader.get_branch()) {
	case LAND:
		leaders = &generals;
		break;
	case NAVAL:
		leaders = &admirals;
		break;
	default:
		spdlog::error_s(
			"Trying to remove leader \"{}\" with invalid branch {} from country {}",
			leader.get_name(), static_cast<uint32_t>(leader.get_branch()), *this
		);
		return false;
	}

	const typename memory::vector<LeaderInstance*>::const_iterator it = std::find(leaders->begin(), leaders->end(), &leader);

	if (it != leaders->end()) {
		leaders->erase(it);
		return true;
	} else {
		spdlog::error_s(
			"Trying to remove non-existent {} \"{}\" from country {}",
			 get_branched_leader_name(leader.get_branch()), leader.get_name(), *this
		);
		return false;
	}
}

bool CountryInstance::has_leader_with_name(std::string_view name) const {
	const auto check_leaders = [&name](memory::vector<LeaderInstance*> const& leaders) -> bool {
		for (LeaderInstance const* leader : leaders) {
			if (leader->get_name() == name) {
				return true;
			}
		}
		return false;
	};

	return check_leaders(generals) || check_leaders(admirals);
}

template<unit_branch_t Branch>
bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<Branch> const& unit_type, technology_unlock_level_t unlock_level_change) {
	IndexedFlatMap<UnitTypeBranched<Branch>, technology_unlock_level_t>& unlocked_unit_types = get_unit_type_unlock_levels<Branch>();
	technology_unlock_level_t& unlock_level = unlocked_unit_types.at(unit_type);

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for unit type {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			 unit_type, *this, static_cast<int64_t>(unlock_level),
			 static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	if constexpr (Branch != unit_branch_t::LAND) {
		unlock_level += unlock_level_change;
	} else {
		bool was_unlocked = is_unit_type_unlocked(unit_type);
		unlock_level += unlock_level_change;
		bool is_unlocked = is_unit_type_unlocked(unit_type);

		if (was_unlocked != is_unlocked) {
			if (was_unlocked) {
				//recalculate entirely
				allowed_regiment_cultures = regiment_allowed_cultures_t::NO_CULTURES;
				for (RegimentType const& regiment_type : unlocked_unit_types.get_keys()) {
					if (!is_unit_type_unlocked(regiment_type)) {
						continue;
					}

					allowed_regiment_cultures = RegimentType::allowed_cultures_get_most_permissive(
						allowed_regiment_cultures,
						regiment_type.get_allowed_cultures()
					);
				}
			} else {
				allowed_regiment_cultures = RegimentType::allowed_cultures_get_most_permissive(
					allowed_regiment_cultures,
					unit_type.get_allowed_cultures()
				);
			}
		}
	}

	return true;
}

template bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<unit_branch_t::LAND> const&, technology_unlock_level_t);
template bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<unit_branch_t::NAVAL> const&, technology_unlock_level_t);

bool CountryInstance::modify_unit_type_unlock(UnitType const& unit_type, technology_unlock_level_t unlock_level_change) {
	using enum unit_branch_t;

	switch (unit_type.get_branch()) {
	case LAND:
		return modify_unit_type_unlock(static_cast<UnitTypeBranched<LAND> const&>(unit_type), unlock_level_change);
	case NAVAL:
		return modify_unit_type_unlock(static_cast<UnitTypeBranched<NAVAL> const&>(unit_type), unlock_level_change);
	default:
		spdlog::error_s(
			"Attempted to change unlock level for unit type \"{}\" with invalid branch {} for country {}",
			unit_type, static_cast<uint32_t>(unit_type.get_branch()), *this
		);
		return false;
	}
}

bool CountryInstance::unlock_unit_type(UnitType const& unit_type) {
	return modify_unit_type_unlock(unit_type, 1);
}

bool CountryInstance::is_unit_type_unlocked(UnitType const& unit_type) const {
	using enum unit_branch_t;

	switch (unit_type.get_branch()) {
	case LAND:
		return regiment_type_unlock_levels.at(static_cast<UnitTypeBranched<LAND> const&>(unit_type)) > 0;
	case NAVAL:
		return ship_type_unlock_levels.at(static_cast<UnitTypeBranched<NAVAL> const&>(unit_type)) > 0;
	default:
		spdlog::error_s(
			"Attempted to check if unit type \"{}\" with invalid branch {} is unlocked for country {}",
			unit_type, static_cast<uint32_t>(unit_type.get_branch()), *this
		);
		return false;
	}
}

bool CountryInstance::modify_building_type_unlock(
	BuildingType const& building_type, technology_unlock_level_t unlock_level_change
) {
	technology_unlock_level_t& unlock_level = building_type_unlock_levels.at(building_type);

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for building type {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			building_type, *this, static_cast<int64_t>(unlock_level), static_cast<int64_t>(unlock_level_change), 
			static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	unlock_level += unlock_level_change;

	if (building_type.get_production_type() != nullptr) {
		good_instance_manager.enable_good(building_type.get_production_type()->get_output_good());
	}

	return true;
}

bool CountryInstance::unlock_building_type(BuildingType const& building_type) {
	return modify_building_type_unlock(building_type, 1);
}

bool CountryInstance::is_building_type_unlocked(BuildingType const& building_type) const {
	return building_type_unlock_levels.at(building_type) > 0;
}

bool CountryInstance::modify_crime_unlock(Crime const& crime, technology_unlock_level_t unlock_level_change) {
	technology_unlock_level_t& unlock_level = crime_unlock_levels.at(crime);

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for crime {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			crime, *this, static_cast<int64_t>(unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	unlock_level += unlock_level_change;

	return true;
}

bool CountryInstance::unlock_crime(Crime const& crime) {
	return modify_crime_unlock(crime, 1);
}

bool CountryInstance::is_crime_unlocked(Crime const& crime) const {
	return crime_unlock_levels.at(crime) > 0;
}

bool CountryInstance::modify_gas_attack_unlock(technology_unlock_level_t unlock_level_change) {
	// This catches subtracting below 0 or adding above the int types maximum value
	if (gas_attack_unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for gas attack in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			*this, static_cast<int64_t>(gas_attack_unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(gas_attack_unlock_level + unlock_level_change)
		);
		return false;
	}

	gas_attack_unlock_level += unlock_level_change;

	return true;
}

bool CountryInstance::unlock_gas_attack() {
	return modify_gas_attack_unlock(1);
}

bool CountryInstance::is_gas_attack_unlocked() const {
	return gas_attack_unlock_level > 0;
}

bool CountryInstance::modify_gas_defence_unlock(technology_unlock_level_t unlock_level_change) {
	// This catches subtracting below 0 or adding above the int types maximum value
	if (gas_defence_unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for gas defence in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			*this, static_cast<int64_t>(gas_defence_unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(gas_defence_unlock_level + unlock_level_change)
		);
		return false;
	}

	gas_defence_unlock_level += unlock_level_change;

	return true;
}

bool CountryInstance::unlock_gas_defence() {
	return modify_gas_defence_unlock(1);
}

bool CountryInstance::is_gas_defence_unlocked() const {
	return gas_defence_unlock_level > 0;
}

bool CountryInstance::modify_unit_variant_unlock(unit_variant_t unit_variant, technology_unlock_level_t unlock_level_change) {
	if (unit_variant < 1) {
		spdlog::error_s("Trying to modify unlock level for default unit variant 0");
		return false;
	}

	if (unit_variant_unlock_levels.size() < unit_variant) {
		unit_variant_unlock_levels.resize(unit_variant);
	}

	technology_unlock_level_t& unlock_level = unit_variant_unlock_levels[unit_variant - 1];

	bool ret = true;

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for unit variant {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			static_cast<uint64_t>(unit_variant), *this, static_cast<int64_t>(unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		ret = false;
	} else {
		unlock_level += unlock_level_change;
	}

	while (!unit_variant_unlock_levels.empty() && unit_variant_unlock_levels.back() < 1) {
		unit_variant_unlock_levels.pop_back();
	}

	return ret;
}

bool CountryInstance::unlock_unit_variant(unit_variant_t unit_variant) {
	return modify_unit_variant_unlock(unit_variant, 1);
}

unit_variant_t CountryInstance::get_max_unlocked_unit_variant() const {
	return unit_variant_unlock_levels.size();
}

bool CountryInstance::modify_technology_unlock(
	Technology const& technology, technology_unlock_level_t unlock_level_change
) {
	technology_unlock_level_t& unlock_level = technology_unlock_levels.at(technology);

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for technology {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			technology, *this, static_cast<int64_t>(unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	unlock_level += unlock_level_change;

	bool ret = true;

	// TODO - bool unciv_military ?

	if (technology.get_unit_variant().has_value()) {
		ret &= modify_unit_variant_unlock(*technology.get_unit_variant(), unlock_level_change);
	}
	for (UnitType const* unit : technology.get_activated_units()) {
		ret &= modify_unit_type_unlock(*unit, unlock_level_change);
	}
	for (BuildingType const* building : technology.get_activated_buildings()) {
		ret &= modify_building_type_unlock(*building, unlock_level_change);
	}

	return ret;
}

bool CountryInstance::set_technology_unlock_level(
	Technology const& technology, technology_unlock_level_t unlock_level
) {
	const technology_unlock_level_t unlock_level_change = unlock_level - technology_unlock_levels.at(technology);
	return unlock_level_change != 0 ? modify_technology_unlock(technology, unlock_level_change) : true;
}

bool CountryInstance::unlock_technology(Technology const& technology) {
	return modify_technology_unlock(technology, 1);
}

bool CountryInstance::is_technology_unlocked(Technology const& technology) const {
	return technology_unlock_levels.at(technology) > 0;
}

bool CountryInstance::modify_invention_unlock(
	Invention const& invention, technology_unlock_level_t unlock_level_change
) {
	technology_unlock_level_t& unlock_level = invention_unlock_levels.at(invention);

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		spdlog::error_s(
			"Attempted to change unlock level for invention {} in country {} to invalid value: current level = {}, change = {}, invalid new value = {}",
			invention, *this, static_cast<int64_t>(unlock_level),
			static_cast<int64_t>(unlock_level_change), static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	const bool invention_was_unlocked = unlock_level > 0;
	unlock_level += unlock_level_change;
	if (invention_was_unlocked != (unlock_level > 0)) {
		if (invention_was_unlocked) {
			inventions_count-=1;
		} else {
			inventions_count+=1;
		}
	}

	bool ret = true;

	// TODO - handle invention.is_news()

	for (UnitType const* unit : invention.get_activated_units()) {
		ret &= modify_unit_type_unlock(*unit, unlock_level_change);
	}
	for (BuildingType const* building : invention.get_activated_buildings()) {
		ret &= modify_building_type_unlock(*building, unlock_level_change);
	}
	for (Crime const* crime : invention.get_enabled_crimes()) {
		ret &= modify_crime_unlock(*crime, unlock_level_change);
	}
	if (invention.will_unlock_gas_attack()) {
		ret &= modify_gas_attack_unlock(unlock_level_change);
	}
	if (invention.will_unlock_gas_defence()) {
		ret &= modify_gas_defence_unlock(unlock_level_change);
	}

	return ret;
}

bool CountryInstance::set_invention_unlock_level(
	Invention const& invention, technology_unlock_level_t unlock_level
) {
	const technology_unlock_level_t unlock_level_change = unlock_level - invention_unlock_levels.at(invention);
	return unlock_level_change != 0 ? modify_invention_unlock(invention, unlock_level_change) : true;
}

bool CountryInstance::unlock_invention(Invention const& invention) {
	return modify_invention_unlock(invention, 1);
}

bool CountryInstance::is_invention_unlocked(Invention const& invention) const {
	return invention_unlock_levels.at(invention) > 0;
}

bool CountryInstance::is_primary_culture(Culture const& culture) const {
	return &culture == primary_culture;
}

bool CountryInstance::is_accepted_culture(Culture const& culture) const {
	return accepted_cultures.contains(&culture);
}

bool CountryInstance::is_primary_or_accepted_culture(Culture const& culture) const {
	return is_primary_culture(culture) || is_accepted_culture(culture);
}

fixed_point_t CountryInstance::calculate_research_cost(Technology const& technology) const {
	// TODO - what if research bonus is -100%? Divide by 0 -> infinite cost?
	return technology.get_cost() / (fixed_point_t::_1 + get_modifier_effect_value(
		*modifier_effect_cache.get_research_bonus_effects(technology.get_area().get_folder())
	));
}

bool CountryInstance::can_research_tech(Technology const& technology, const Date today) const {
	Technology const* current_research_copy = current_research.get_untracked();
	if (
		technology.get_year() > today.get_year()
		|| !is_civilised()
		|| is_technology_unlocked(technology)
		|| (current_research_copy && technology == *current_research_copy)
	) {
		return false;
	}

	const Technology::area_index_t index_in_area = technology.get_index_in_area();

	return index_in_area == 0 || is_technology_unlocked(*technology.get_area().get_technologies()[index_in_area - 1]);
}

void CountryInstance::start_research(Technology const& technology, const Date today) {
	if (OV_unlikely(!can_research_tech(technology, today))) {
		spdlog::warn_s(
			"Attempting to start research for country \"{}\" on technology \"{}\" - cannot research this tech!",
			*this, technology
		);
		return;
	}

	current_research.set(&technology);
	invested_research_points.set(0);

	_update_current_tech(today);
}

void CountryInstance::apply_foreign_investments(
	fixed_point_map_t<CountryDefinition const*> const& investments, CountryInstanceManager const& country_instance_manager
) {
	for (auto const& [country, money_invested] : investments) {
		foreign_investments[&country_instance_manager.get_country_instance_by_definition(*country)] = money_invested;
	}
}

bool CountryInstance::apply_history_to_country(
	CountryHistoryEntry const& entry,
	CountryInstanceManager const& country_instance_manager,
	FlagStrings& global_flags,
	MapInstance& map_instance
) {
	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
		}
	};
	constexpr auto set_optional_state = []<typename T>(MutableState<T>& target, std::optional<T> const& source) {
		if (source) {
			target.set(*source);
		}
	};

	bool ret = true;

	set_optional(primary_culture, entry.get_primary_culture());
	for (auto const& [culture, add] : entry.get_accepted_cultures()) {
		if (add) {
			ret &= add_accepted_culture(*culture);
		} else {
			ret &= remove_accepted_culture(*culture);
		}
	}
	set_optional(religion, entry.get_religion());
	if (entry.get_ruling_party()) {
		ret &= set_ruling_party(**entry.get_ruling_party());
	}
	set_optional(last_election, entry.get_last_election());
	upper_house_proportion_by_ideology.copy_values_from(entry.get_upper_house_proportion_by_ideology());
	if (entry.get_capital()) {
		capital = &map_instance.get_province_instance_by_definition(**entry.get_capital());
	}
	set_optional_state(government_type, entry.get_government_type());
	set_optional_state(plurality, entry.get_plurality());
	set_optional_state(national_value, entry.get_national_value());
	if (entry.is_civilised()) {
		country_status = *entry.is_civilised() ? COUNTRY_STATUS_CIVILISED : COUNTRY_STATUS_UNCIVILISED;
	}
	set_optional_state(prestige, entry.get_prestige());
	for (Reform const* reform : entry.get_reforms()) {
		ret &= add_reform(*reform);
	}
	set_optional_state(tech_school, entry.get_tech_school());
	constexpr auto set_bool_map_to_indexed_map =
		[]<typename T>(IndexedFlatMap<T, bool>& target, ordered_map<T const*, bool> source) {
			for (auto const& [key, value] : source) {
				target[*key] = value;
			}
		};

	for (auto const& [technology, level] : entry.get_technologies()) {
		ret &= set_technology_unlock_level(*technology, level);
	}
	for (auto const& [invention, activated] : entry.get_inventions()) {
		ret &= set_invention_unlock_level(*invention, activated ? 1 : 0);
	}

	apply_foreign_investments(entry.get_foreign_investment(), country_instance_manager);

	set_optional(releasable_vassal, entry.is_releasable_vassal());

	// TODO - entry.get_colonial_points();

	ret &= apply_flag_map(entry.get_country_flags(), true);
	ret &= global_flags.apply_flag_map(entry.get_global_flags(), true);
	flag_overrides_by_government_type.copy_values_from(entry.get_flag_overrides_by_government_type());
	for (Decision const* decision : entry.get_decisions()) {
		// TODO - take decision
	}

	return ret;
}

void CountryInstance::_update_production() {
	// Calculate industrial power from states and foreign investments
	fixed_point_t industrial_power_running_total = 0;
	industrial_power_from_states.clear();
	industrial_power_from_investments.clear();

	for (State const* state : states) {
		const fixed_point_t state_industrial_power = state->get_industrial_power();
		if (state_industrial_power != 0) {
			industrial_power += state_industrial_power;
			industrial_power_from_states.emplace_back(state, state_industrial_power);
		}
	}

	for (auto const& [country, money_invested] : foreign_investments) {
		if (country->exists()) {
			const fixed_point_t investment_industrial_power = fixed_point_t::mul_div(
				money_invested,
				country_defines.get_country_investment_industrial_score_factor(),
				100
			);

			if (investment_industrial_power != 0) {
				industrial_power += investment_industrial_power;
				industrial_power_from_investments.emplace_back(country, investment_industrial_power);
			}
		}
	}

	industrial_power.set(industrial_power_running_total);

	std::stable_sort(
		industrial_power_from_states.begin(), industrial_power_from_states.end(),
		[](auto const& a, auto const& b) -> bool { return a.second > b.second; }
	);
	std::stable_sort(
		industrial_power_from_investments.begin(), industrial_power_from_investments.end(),
		[](auto const& a, auto const& b) -> bool { return a.second > b.second; }
	);
}

void CountryInstance::_update_current_tech(const Date today) {
	Technology const* current_research_copy = current_research.get_untracked();
	if (current_research_copy == nullptr) {
		return;
	}

	current_research_cost.set(
		calculate_research_cost(*current_research_copy)
	);

	const fixed_point_t daily_research_points_copy = daily_research_points.get_untracked();
	if (daily_research_points_copy > 0) {
		expected_research_completion_date.set(
			today
			+ static_cast<Timespan>(
				(
					(current_research_cost.get_untracked() - invested_research_points.get_untracked())
					/ daily_research_points_copy
				).ceil<int64_t>()
			)
		);
	} else {
		expected_research_completion_date.set(fallback_date_for_never_completing_research);
	}
}

void CountryInstance::_update_technology(const Date today) {
	if (research_point_stockpile.get_untracked() < 0) {
		research_point_stockpile.set(0);
	}

	daily_research_points += get_modifier_effect_value(*modifier_effect_cache.get_research_points());
	daily_research_points *= fixed_point_t::_1 +
		get_modifier_effect_value(*modifier_effect_cache.get_research_points_modifier()) +
		get_modifier_effect_value(*modifier_effect_cache.get_increase_research());

	if (daily_research_points.get_untracked() < 0) {
		daily_research_points.set(0);
	}

	_update_current_tech(today);
}

void CountryInstance::_update_politics() {

}

void CountryInstance::_update_population() {
	clear_pops_aggregate();

	for (State* const state : states) {
		add_pops_aggregate(*state);
	}

	normalise_pops_aggregate();

	daily_research_points.set(0);
	monthly_leadership_points = 0;
	research_points_from_pop_types.clear();
	leadership_points_from_pop_types.clear();

	for (auto const& [pop_type, pop_size] : get_population_by_type()) {
		if (pop_type.get_research_leadership_optimum() > 0 && pop_size > 0) {
			const fixed_point_t factor = std::min(
				pop_size / (get_total_population() * pop_type.get_research_leadership_optimum()), fixed_point_t::_1
			);

			if (pop_type.get_research_points() != 0) {
				const fixed_point_t research_points = pop_type.get_research_points() * factor;
				research_points_from_pop_types[&pop_type] = research_points;
				daily_research_points += research_points;
			}

			if (pop_type.get_leadership_points() != 0) {
				const fixed_point_t leadership_points = pop_type.get_leadership_points() * factor;
				leadership_points_from_pop_types[&pop_type] = leadership_points;
				monthly_leadership_points = leadership_points;
			}
		}
	}

	// TODO - update national focus capacity
}

void CountryInstance::_update_diplomacy() {
	// TODO - add prestige from modifiers
	// TODO - update diplomatic points and colonial power
}

void CountryInstance::_update_military() {
	regiment_count = 0;

	for (ArmyInstance const* army : armies) {
		regiment_count += army->get_unit_count();
	}

	ship_count = 0;
	total_consumed_ship_supply = 0;

	for (NavyInstance const* navy : navies) {
		ship_count += navy->get_unit_count();
		total_consumed_ship_supply += navy->get_total_consumed_supply();
	}

	// Calculate military power from land, sea, and leaders

	size_t deployed_non_mobilised_regiments = 0;
	for (ArmyInstance const* army : armies) {
		for (RegimentInstance const* regiment : army->get_regiment_instances()) {
			if (!regiment->is_mobilised()) {
				deployed_non_mobilised_regiments++;
			}
		}
	}

	supply_consumption = fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_supply_consumption());

	const size_t regular_army_size = std::min(4 * deployed_non_mobilised_regiments, get_max_supported_regiment_count());

	fixed_point_t sum_of_regiment_type_stats = 0;
	for (RegimentType const& regiment_type : unit_type_manager.get_regiment_types()) {
		// TODO - apply country/tech modifiers to regiment stats
		sum_of_regiment_type_stats += (
			regiment_type.get_attack() + regiment_type.get_defence() /*+ land_attack_modifier + land_defense_modifier*/
		) * regiment_type.get_discipline();
	}

	military_power_from_land.set(
		supply_consumption * fixed_point_t::parse(regular_army_size) * sum_of_regiment_type_stats
		/ fixed_point_t::parse(7 * (1 + unit_type_manager.get_regiment_type_count()))
	);

	if (disarmed) {
		military_power_from_land *= diplomacy_defines.get_disarmed_penalty();
	}

	fixed_point_t military_power_from_sea_running_total = 0;
	for (NavyInstance const* navy : navies) {
		for (ShipInstance const* ship : navy->get_ship_instances()) {
			ShipType const& ship_type = ship->get_ship_type();

			if (ship_type.is_capital()) {

				// TODO - include gun power and hull modifiers + naval attack and defense modifiers

				military_power_from_sea_running_total += (ship_type.get_gun_power() /*+ naval_attack_modifier*/)
					* (ship_type.get_hull() /* + naval_defense_modifier*/);
			}
		}
	}
	military_power_from_sea.set(military_power_from_sea_running_total / 250);

	military_power_from_leaders.set(
		fixed_point_t::parse(std::min(get_leader_count(), deployed_non_mobilised_regiments))
	);

	// Mobilisation calculations
	mobilisation_impact = get_modifier_effect_value(*modifier_effect_cache.get_mobilization_impact());
	mobilisation_economy_impact = get_modifier_effect_value(*modifier_effect_cache.get_mobilisation_economy_impact_tech()) +
		get_modifier_effect_value(*modifier_effect_cache.get_mobilisation_economy_impact_country());

	// TODO - use country_defines.get_min_mobilize_limit(); (wiki: "lowest maximum of brigades you can mobilize. (by default 3)")

	mobilisation_max_regiment_count =
		((fixed_point_t::_1 + mobilisation_impact) * fixed_point_t::parse(regiment_count)).floor<size_t>();

	mobilisation_potential_regiment_count = 0; // TODO - calculate max regiments from poor citizens
	if (mobilisation_potential_regiment_count > mobilisation_max_regiment_count) {
		mobilisation_potential_regiment_count = mobilisation_max_regiment_count;
	}

	// Limit max war exhaustion to non-negative values. This technically diverges from the base game where
	// max war exhaustion can be negative, but in such cases the war exhaustion clamping behaviour is
	// very buggy and regardless it doesn't seem to make any modifier effect contributions.
	war_exhaustion_max = std::max(
		get_modifier_effect_value(*modifier_effect_cache.get_max_war_exhaustion()), fixed_point_t::_0
	);

	organisation_regain = fixed_point_t::_1 +
		get_modifier_effect_value(*modifier_effect_cache.get_org_regain()) +
		get_modifier_effect_value(*modifier_effect_cache.get_morale_global());

	land_organisation = fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_land_organisation());
	naval_organisation = fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_naval_organisation());

	land_unit_start_experience = get_modifier_effect_value(*modifier_effect_cache.get_regular_experience_level());
	naval_unit_start_experience = land_unit_start_experience;
	land_unit_start_experience += get_modifier_effect_value(*modifier_effect_cache.get_land_unit_start_experience());
	naval_unit_start_experience += get_modifier_effect_value(*modifier_effect_cache.get_naval_unit_start_experience());

	recruit_time = fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_unit_recruitment_time());
	combat_width = ( //
		fixed_point_t::parse(military_defines.get_base_combat_width()) +
		get_modifier_effect_value(*modifier_effect_cache.get_combat_width_additive())
	).floor<int32_t>();
	dig_in_cap = get_modifier_effect_value(*modifier_effect_cache.get_dig_in_cap()).floor<int32_t>();
	military_tactics = military_defines.get_base_military_tactics() +
		get_modifier_effect_value(*modifier_effect_cache.get_military_tactics());

	if (leadership_point_stockpile < 0) {
		leadership_point_stockpile = 0;
	}
	create_leader_count = (leadership_point_stockpile / military_defines.get_leader_recruit_cost()).floor<int32_t>();

	monthly_leadership_points += get_modifier_effect_value(*modifier_effect_cache.get_leadership());
	monthly_leadership_points *= fixed_point_t::_1 +
		get_modifier_effect_value(*modifier_effect_cache.get_leadership_modifier());

	if (monthly_leadership_points < 0) {
		monthly_leadership_points = 0;
	}

	// TODO - update max_ship_supply
}

bool CountryInstance::update_rule_set() {
	rule_set.clear();
	CountryParty const* ruling_party_copy = ruling_party.get_untracked();
	if (ruling_party_copy != nullptr) {
		for (PartyPolicy const* party_policy : ruling_party_copy->get_policies().get_values()) {
			if (party_policy != nullptr) {
				rule_set |= party_policy->get_rules();
			}
		}
	}

	for (Reform const* reform : reforms.get_values()) {
		if (reform != nullptr) {
			rule_set |= reform->get_rules();
		}
	}

	return rule_set.trim_and_resolve_conflicts(true);
}

static constexpr Modifier const& get_country_status_static_effect(
	CountryInstance::country_status_t country_status, StaticModifierCache const& static_modifier_cache
) {
	using enum CountryInstance::country_status_t;

	switch (country_status) {
	case COUNTRY_STATUS_GREAT_POWER:     return static_modifier_cache.get_great_power();
	case COUNTRY_STATUS_SECONDARY_POWER: return static_modifier_cache.get_secondary_power();
	case COUNTRY_STATUS_CIVILISED:       return static_modifier_cache.get_civilised();
	default:                             return static_modifier_cache.get_uncivilised();
	}
}

void CountryInstance::update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache) {
	// Update sum of national modifiers
	modifier_sum.clear();

	// Add static modifiers
	modifier_sum.add_modifier(static_modifier_cache.get_base_modifier());
	modifier_sum.add_modifier(get_country_status_static_effect(country_status, static_modifier_cache));
	if (is_disarmed()) {
		modifier_sum.add_modifier(static_modifier_cache.get_disarming());
	}
	modifier_sum.add_modifier(static_modifier_cache.get_war_exhaustion(), war_exhaustion);
	modifier_sum.add_modifier(static_modifier_cache.get_infamy(), infamy.get_untracked());
	modifier_sum.add_modifier(static_modifier_cache.get_literacy(), get_average_literacy());
	modifier_sum.add_modifier(static_modifier_cache.get_plurality(), plurality.get_untracked());
	modifier_sum.add_modifier(is_at_war() ? static_modifier_cache.get_war() : static_modifier_cache.get_peace());
	// TODO - difficulty modifiers, debt_default_to, bad_debtor, generalised_debt_default,
	//        total_occupation, total_blockaded, in_bankruptcy

	// TODO - handle triggered modifiers

	CountryParty const* ruling_party_copy = ruling_party.get_untracked();
	if (ruling_party_copy != nullptr) {
		for (PartyPolicy const* party_policy : ruling_party_copy->get_policies().get_values()) {
			// The ruling party's issues here could be null as they're stored in an IndexedFlatMap which has
			// values for every PartyPolicyGroup regardless of whether or not they have a policy set.
			if (party_policy != nullptr) {
				modifier_sum.add_modifier(*party_policy);
			}
		}
	}

	for (Reform const* reform : reforms.get_values()) {
		// The country's reforms here could be null as they're stored in an IndexedFlatMap which has
		// values for every ReformGroup regardless of whether or not they have a reform set.
		if (reform != nullptr) {
			modifier_sum.add_modifier(*reform);
		}
	}

	TechnologySchool const* tech_school_copy = tech_school.get_untracked();
	if (tech_school_copy != nullptr) {
		modifier_sum.add_modifier(*tech_school_copy);
	}

	for (Technology const& technology : technology_unlock_levels.get_keys()) {
		if (is_technology_unlocked(technology)) {
			modifier_sum.add_modifier(technology);
		}
	}

	for (Invention const& invention : invention_unlock_levels.get_keys()) {
		if (is_invention_unlocked(invention)) {
			modifier_sum.add_modifier(invention);
		}
	}

	// Erase expired event modifiers and add non-expired ones to the sum
	std::erase_if(event_modifiers, [this, today](ModifierInstance const& modifier) -> bool {
		if (today <= modifier.get_expiry_date()) {
			modifier_sum.add_modifier(*modifier.get_modifier());
			return false;
		} else {
			return true;
		}
	});

	NationalValue const* national_value_copy = national_value.get_untracked();
	if (national_value_copy != nullptr) {
		modifier_sum.add_modifier(*national_value_copy);
	}

	// TODO - calculate stats for each unit type (locked and unlocked)
}

void CountryInstance::contribute_province_modifier_sum(ModifierSum const& province_modifier_sum) {
	modifier_sum.add_modifier_sum(province_modifier_sum);
}

fixed_point_t CountryInstance::get_modifier_effect_value(ModifierEffect const& effect) const {
	return modifier_sum.get_modifier_effect_value(effect);
}

fixed_point_t CountryInstance::get_yesterdays_import_value(DependencyTracker& tracker) {
	return PopsAggregate::get_yesterdays_import_value(tracker);
}

void CountryInstance::update_gamestate(const Date today, MapInstance& map_instance) {
	if (is_civilised()) {
		civilisation_progress = 0;
	} else {
		civilisation_progress = get_modifier_effect_value(*modifier_effect_cache.get_civilization_progress_modifier());

		if (civilisation_progress <= PRIMITIVE_CIVILISATION_PROGRESS) {
			country_status = COUNTRY_STATUS_PRIMITIVE;
		} else if (civilisation_progress <= UNCIVILISED_CIVILISATION_PROGRESS) {
			country_status = COUNTRY_STATUS_UNCIVILISED;
		} else {
			country_status = COUNTRY_STATUS_PARTIALLY_CIVILISED;
		}
	}

	owns_colonial_province = std::any_of(
		owned_provinces.begin(), owned_provinces.end(), std::bind_front(&ProvinceInstance::is_colonial_province)
	);

	{
		has_unowned_cores = false;
		owned_cores_controlled_proportion = 0;
		int32_t owned_core_province_count = 0;

		for (ProvinceInstance const* core_province : core_provinces) {
			if (core_province->get_owner() == this) {
				owned_core_province_count++;

				if (core_province->get_controller() == this) {
					owned_cores_controlled_proportion++;
				}
			} else {
				has_unowned_cores = true;
			}
		}

		if (owned_cores_controlled_proportion != 0) {
			owned_cores_controlled_proportion /= owned_core_province_count;
		}
	}

	occupied_provinces_proportion = 0;
	port_count = 0;
	neighbouring_countries.clear();

	Continent const* capital_continent = capital != nullptr ? capital->get_province_definition().get_continent() : nullptr;

	coastal = false;
	for (ProvinceInstance* province : owned_provinces) {
		ProvinceDefinition const& province_definition = province->get_province_definition();
		coastal |= province_definition.is_coastal();

		if (province->get_controller() != this) {
			occupied_provinces_proportion++;
		}

		if (province_definition.has_port()) {
			port_count++;
		}

		// TODO - ensure connected_to_capital and is_overseas are updated to false if a province becomes uncolonised
		province->set_connected_to_capital(false);
		province->set_is_overseas(province_definition.get_continent() != capital_continent);

		for (ProvinceDefinition::adjacency_t const& adjacency : province_definition.get_adjacencies()) {
			// TODO - should we limit based on adjacency type? Straits and impassable still work in game,
			// and water provinces don't have an owner so they'll get caught by the later checks anyway.
			CountryInstance* neighbour = map_instance.get_province_instance_by_definition(*adjacency.get_to()).get_owner();
			if (neighbour != nullptr && neighbour != this) {
				neighbouring_countries.insert(neighbour);
			}
		}
	}

	if (occupied_provinces_proportion != 0) {
		occupied_provinces_proportion /= owned_provinces.size();
	}

	if (capital != nullptr) {
		capital->set_connected_to_capital(true);
		memory::vector<ProvinceInstance const*> province_checklist { capital };

		for (size_t index = 0; index < province_checklist.size(); index++) {
			ProvinceInstance const& province = *province_checklist[index];

			for (ProvinceDefinition::adjacency_t const& adjacency : province.get_province_definition().get_adjacencies()) {
				ProvinceInstance& adjacent_province = map_instance.get_province_instance_by_definition(*adjacency.get_to());

				if (adjacent_province.get_owner() == this && !adjacent_province.get_connected_to_capital()) {
					adjacent_province.set_connected_to_capital(true);
					adjacent_province.set_is_overseas(false);
					province_checklist.push_back(&adjacent_province);
				}
			}
		}
	}

	// Order of updates might need to be changed/functions split up to account for dependencies
	// Updates population stats (including research and leadership points from pops)
	_update_population();
	// Calculates industrial power
	_update_production();
	// Calculates daily research points and predicts research completion date
	_update_technology(today);
	// Calculates national military modifiers, army and navy stats, daily leadership points
	_update_military();
	update_country_budget(
		desired_administrator_percentage.get_untracked(),
		get_population_by_type(),
		get_unemployed_pops_by_type(),
		get_states()
	);

	// These don't do anything yet
	_update_politics();
	_update_diplomacy();

	const CountryDefinition::government_colour_map_t::const_iterator it =
		country_definition.get_alternative_colours().find(government_type.get_untracked());

	if (it != country_definition.get_alternative_colours().end()) {
		colour = it.value();
	} else {
		colour = country_definition.get_colour();
	}
}

void CountryInstance::country_tick_before_map(
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	utility::forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_COUNTRY_TICK
	> reusable_vectors,
	memory::vector<size_t>& reusable_index_vector
) {
	country_budget_tick_before_map(
		*this,
		reusable_goods_mask,
		reusable_vectors,
		reusable_index_vector
	);
}

void CountryInstance::country_tick_after_map(const Date today) {
	// Gain daily research points
	research_point_stockpile += daily_research_points.get_untracked();

	Technology const* current_research_copy = current_research.get_untracked();
	if (current_research_copy != nullptr) {
		const fixed_point_t research_points_spent = std::min(
			std::min(
				research_point_stockpile.get_untracked(),
				current_research_cost.get_untracked() - invested_research_points.get_untracked()
			),
			economy_defines.get_max_daily_research()
		);

		research_point_stockpile -= research_points_spent;
		invested_research_points += research_points_spent;

		if (invested_research_points.get_untracked() >= current_research_cost.get_untracked()) {
			unlock_technology(*current_research_copy);
			current_research.set(nullptr);
			invested_research_points.set(0);
			current_research_cost.set(0);
		}
	}

	// Apply maximum research point stockpile limit
	const fixed_point_t max_research_point_stockpile = is_civilised()
		? daily_research_points.get_untracked() * static_cast<int32_t>(Date::DAYS_IN_YEAR)
		: country_defines.get_max_research_points();
	if (research_point_stockpile.get_untracked() > max_research_point_stockpile) {
		research_point_stockpile.set(max_research_point_stockpile);
	}

	// Gain monthly leadership points
	if (today.is_month_start()) {
		leadership_point_stockpile += monthly_leadership_points;
	}

	// TODO - auto create and auto assign leaders

	// Apply maximum leadership point stockpile limit
	const fixed_point_t max_leadership_point_stockpile = military_defines.get_max_leadership_point_stockpile();
	if (leadership_point_stockpile > max_leadership_point_stockpile) {
		leadership_point_stockpile = max_leadership_point_stockpile;
	}

	country_budget_tick_after_map(
		*this,
		today
	);
}

template struct fmt::formatter<OpenVic::CountryInstance>;
