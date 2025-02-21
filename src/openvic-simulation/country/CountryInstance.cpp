#include "CountryInstance.hpp"

#include <cstdint>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/SliderValue.hpp"

using namespace OpenVic;

using enum CountryInstance::country_status_t;

static constexpr colour_t ERROR_COLOUR = colour_t::from_integer(0xFF0000);

CountryInstance::CountryInstance(
	CountryDefinition const* new_country_definition,
	index_t new_index,
	decltype(building_type_unlock_levels)::keys_type const& building_type_keys,
	decltype(technology_unlock_levels)::keys_type const& technology_keys,
	decltype(invention_unlock_levels)::keys_type const& invention_keys,
	decltype(upper_house)::keys_type const& ideology_keys,
	decltype(reforms)::keys_type const& reform_keys,
	decltype(government_flag_overrides)::keys_type const& government_type_keys,
	decltype(crime_unlock_levels)::keys_type const& crime_keys,
	decltype(pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(goods_data)::keys_type const& good_instances_keys,
	decltype(regiment_type_unlock_levels)::keys_type const& regiment_type_unlock_levels_keys,
	decltype(ship_type_unlock_levels)::keys_type const& ship_type_unlock_levels_keys,
	decltype(tax_rate_slider_value_by_strata)::keys_type const& strata_keys,
	GameRulesManager const& new_game_rules_manager,
	GoodInstanceManager& new_good_instance_manager,
	EconomyDefines const& economy_defines
) : FlagStrings { "country" },
	HasIndex { new_index },
	/* Main attributes */
	country_definition { new_country_definition },
	game_rules_manager { new_game_rules_manager },
	colour { ERROR_COLOUR },

	/* Production */
	building_type_unlock_levels { &building_type_keys },

	/* Budget */
	tax_rate_slider_value_by_strata { &strata_keys },

	/* Technology */
	technology_unlock_levels { &technology_keys },
	invention_unlock_levels { &invention_keys },

	/* Politics */
	upper_house { &ideology_keys },
	reforms { &reform_keys },
	government_flag_overrides { &government_type_keys },
	crime_unlock_levels { &crime_keys },

	/* Population */
	population_by_strata { &strata_keys },
	militancy_by_strata { &strata_keys },
	life_needs_fulfilled_by_strata { &strata_keys },
	everyday_needs_fulfilled_by_strata { &strata_keys },
	luxury_needs_fulfilled_by_strata { &strata_keys },
	pop_type_distribution { &pop_type_keys },
	ideology_distribution { &ideology_keys },
	vote_distribution { nullptr },

	/* Trade */
	goods_data { &good_instances_keys },

	/* Diplomacy */

	/* Military */
	regiment_type_unlock_levels { &regiment_type_unlock_levels_keys },
	ship_type_unlock_levels { &ship_type_unlock_levels_keys } {

	// Exclude PROVINCE (local) modifier effects from the country's modifier sum
	modifier_sum.set_this_excluded_targets(ModifierEffect::target_t::PROVINCE);

	// Some sliders need to have their max range limits temporarily set to 1 so they can start with a value of 0.5 or 1.0.
	// The range limits will be corrected on the first gamestate update, and the values will go to the closest valid point.
	for (SliderValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
		tax_rate_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_1());
		tax_rate_slider_value.set_value(fixed_point_t::_0_50());
	}

	// Land, naval and construction spending have minimums defined in EconomyDefines and always have an unmodified max (1.0).
	land_spending_slider_value.set_bounds(economy_defines.get_minimum_land_spending_slider_value(), fixed_point_t::_1());
	land_spending_slider_value.set_value(fixed_point_t::_1());

	naval_spending_slider_value.set_bounds(economy_defines.get_minimum_naval_spending_slider_value(), fixed_point_t::_1());
	naval_spending_slider_value.set_value(fixed_point_t::_1());

	construction_spending_slider_value.set_bounds(
		economy_defines.get_minimum_construction_spending_slider_value(), fixed_point_t::_1()
	);
	construction_spending_slider_value.set_value(fixed_point_t::_1());

	education_spending_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_1());
	education_spending_slider_value.set_value(fixed_point_t::_0_50());

	administration_spending_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_1());
	administration_spending_slider_value.set_value(fixed_point_t::_0_50());

	social_spending_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_1());
	social_spending_slider_value.set_value(fixed_point_t::_1());

	military_spending_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_1());
	military_spending_slider_value.set_value(fixed_point_t::_0_50());

	tariff_rate_slider_value.set_bounds(fixed_point_t::_0(), fixed_point_t::_0());
	tariff_rate_slider_value.set_value(fixed_point_t::_0());

	update_country_definition_based_attributes();

	for (BuildingType const& building_type : *building_type_unlock_levels.get_keys()) {
		if (building_type.is_default_enabled()) {
			unlock_building_type(building_type, new_good_instance_manager);
		}
	}

	for (Crime const& crime : *crime_unlock_levels.get_keys()) {
		if (crime.is_default_active()) {
			unlock_crime(crime);
		}
	}

	for (RegimentType const& regiment_type : *regiment_type_unlock_levels.get_keys()) {
		if (regiment_type.is_active()) {
			unlock_unit_type(regiment_type);
		}
	}

	for (ShipType const& ship_type : *ship_type_unlock_levels.get_keys()) {
		if (ship_type.is_active()) {
			unlock_unit_type(ship_type);
		}
	}
}

std::string_view CountryInstance::get_identifier() const {
	return country_definition->get_identifier();
}

void CountryInstance::update_country_definition_based_attributes() {
	vote_distribution.set_keys(&country_definition->get_parties());
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
	// TODO - implement this properly once we have wars
	return false;
}

bool CountryInstance::is_neighbour(CountryInstance const& country) const {
	return neighbouring_countries.contains(&country);
}

fixed_point_t CountryInstance::get_script_variable(std::string const& variable_name) const {
	const decltype(script_variables)::const_iterator it = script_variables.find(variable_name);

	if (it != script_variables.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

void CountryInstance::set_script_variable(std::string const& variable_name, fixed_point_t value) {
	script_variables[variable_name] = value;
}

void CountryInstance::change_script_variable(std::string const& variable_name, fixed_point_t value) {
	script_variables[variable_name] += value;
}

fixed_point_t CountryInstance::get_issue_support(Issue const& issue) const {
	const decltype(issue_distribution)::const_iterator it = issue_distribution.find(&issue);

	if (it != issue_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t CountryInstance::get_party_support(CountryParty const& party) const {
	if (vote_distribution.has_keys()) {
		return vote_distribution[party];
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t CountryInstance::get_culture_proportion(Culture const& culture) const {
	const decltype(culture_distribution)::const_iterator it = culture_distribution.find(&culture);

	if (it != culture_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t CountryInstance::get_religion_proportion(Religion const& religion) const {
	const decltype(religion_distribution)::const_iterator it = religion_distribution.find(&religion);

	if (it != religion_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

#define ADD_AND_REMOVE(item) \
	bool CountryInstance::add_##item(std::remove_pointer_t<decltype(item##s)::value_type>& new_item) { \
		if (!item##s.emplace(&new_item).second) { \
			Logger::error( \
				"Attempted to add " #item " \"", new_item.get_identifier(), "\" to country ", get_identifier(), \
				": already present!" \
			); \
			return false; \
		} \
		return true; \
	} \
	bool CountryInstance::remove_##item(std::remove_pointer_t<decltype(item##s)::value_type> const& item_to_remove) { \
		if (item##s.erase(&item_to_remove) == 0) { \
			Logger::error( \
				"Attempted to remove " #item " \"", item_to_remove.get_identifier(), "\" from country ", get_identifier(), \
				": not present!" \
			); \
			return false; \
		} \
		return true; \
	} \
	bool CountryInstance::has_##item(std::remove_pointer_t<decltype(item##s)::value_type> const& item) const { \
		return item##s.contains(&item); \
	}

ADD_AND_REMOVE(owned_province)
ADD_AND_REMOVE(controlled_province)
ADD_AND_REMOVE(core_province)
ADD_AND_REMOVE(state)
ADD_AND_REMOVE(accepted_culture)

#undef ADD_AND_REMOVE

bool CountryInstance::set_upper_house(Ideology const* ideology, fixed_point_t popularity) {
	if (ideology != nullptr) {
		upper_house[*ideology] = popularity;
		return true;
	} else {
		Logger::error("Trying to set null ideology in upper house of ", get_identifier());
		return false;
	}
}

bool CountryInstance::set_ruling_party(CountryParty const& new_ruling_party) {
	if (ruling_party != &new_ruling_party) {
		ruling_party = &new_ruling_party;

		return update_rule_set();
	} else {
		return true;
	}
}

bool CountryInstance::add_reform(Reform const& new_reform) {
	ReformGroup const& reform_group = new_reform.get_reform_group();
	decltype(reforms)::value_ref_type reform = reforms[reform_group];

	if (reform != &new_reform) {
		if (reform_group.is_administrative()) {
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

void CountryInstance::set_strata_tax_rate_slider_value(Strata const& strata, const fixed_point_t new_value) {
	tax_rate_slider_value_by_strata[strata].set_value(new_value);
}

void CountryInstance::set_land_spending_slider_value(const fixed_point_t new_value) {
	land_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_naval_spending_slider_value(const fixed_point_t new_value) {
	naval_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_construction_spending_slider_value(const fixed_point_t new_value) {
	construction_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_education_spending_slider_value(const fixed_point_t new_value) {
	education_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_administration_spending_slider_value(const fixed_point_t new_value) {
	administration_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_social_spending_slider_value(const fixed_point_t new_value) {
	social_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_military_spending_slider_value(const fixed_point_t new_value) {
	military_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_tariff_rate_slider_value(const fixed_point_t new_value) {
	tariff_rate_slider_value.set_value(new_value);
}

void CountryInstance::change_war_exhaustion(fixed_point_t delta) {
	war_exhaustion = std::clamp(war_exhaustion + delta, fixed_point_t::_0(), war_exhaustion_max);
}

bool CountryInstance::add_unit_instance_group(UnitInstanceGroup& group) {
	using enum UnitType::branch_t;

	switch (group.get_branch()) {
	case LAND:
		armies.push_back(static_cast<ArmyInstance*>(&group));
		return true;
	case NAVAL:
		navies.push_back(static_cast<NavyInstance*>(&group));
		return true;
	default:
		Logger::error(
			"Trying to add unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " to country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::remove_unit_instance_group(UnitInstanceGroup const& group) {
	const auto remove_from_vector = [this, &group]<UnitType::branch_t Branch>(
		std::vector<UnitInstanceGroupBranched<Branch>*>& unit_instance_groups
	) -> bool {
		const typename std::vector<UnitInstanceGroupBranched<Branch>*>::const_iterator it =
			std::find(unit_instance_groups.begin(), unit_instance_groups.end(), &group);

		if (it != unit_instance_groups.end()) {
			unit_instance_groups.erase(it);
			return true;
		} else {
			Logger::error(
				"Trying to remove non-existent ", UnitType::get_branched_unit_group_name(Branch), " \"",
				group.get_name(), "\" from country ", get_identifier()
			);
			return false;
		}
	};

	using enum UnitType::branch_t;

	switch (group.get_branch()) {
	case LAND:
		return remove_from_vector(armies);
	case NAVAL:
		return remove_from_vector(navies);
	default:
		Logger::error(
			"Trying to remove unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " from country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::add_leader(LeaderInstance& leader) {
	using enum UnitType::branch_t;

	switch (leader.get_branch()) {
	case LAND:
		generals.push_back(&leader);
		return true;
	case NAVAL:
		admirals.push_back(&leader);
		return true;
	default:
		Logger::error(
			"Trying to add leader \"", leader.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(leader.get_branch()), " to country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::remove_leader(LeaderInstance const& leader) {
	using enum UnitType::branch_t;

	std::vector<LeaderInstance*>* leaders;

	switch (leader.get_branch()) {
	case LAND:
		leaders = &generals;
		break;
	case NAVAL:
		leaders = &admirals;
		break;
	default:
		Logger::error(
			"Trying to remove leader \"", leader.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(leader.get_branch()), " from country ", get_identifier()
		);
		return false;
	}

	const typename std::vector<LeaderInstance*>::const_iterator it = std::find(leaders->begin(), leaders->end(), &leader);

	if (it != leaders->end()) {
		leaders->erase(it);
		return true;
	} else {
		Logger::error(
			"Trying to remove non-existent ", UnitType::get_branched_leader_name(leader.get_branch()), " \"",
			leader.get_name(), "\" from country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::has_leader_with_name(std::string_view name) const {
	const auto check_leaders = [&name](std::vector<LeaderInstance*> const& leaders) -> bool {
		for (LeaderInstance const* leader : leaders) {
			if (leader->get_name() == name) {
				return true;
			}
		}
		return false;
	};

	return check_leaders(generals) || check_leaders(admirals);
}

template<UnitType::branch_t Branch>
bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<Branch> const& unit_type, unlock_level_t unlock_level_change) {
	IndexedMap<UnitTypeBranched<Branch>, unlock_level_t>& unlocked_unit_types = get_unit_type_unlock_levels<Branch>();

	typename IndexedMap<UnitTypeBranched<Branch>, unlock_level_t>::value_ref_type unlock_level =
		unlocked_unit_types[unit_type];

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for unit type ", unit_type.get_identifier(), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	unlock_level += unlock_level_change;

	return true;
}

template bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<UnitType::branch_t::LAND> const&, unlock_level_t);
template bool CountryInstance::modify_unit_type_unlock(UnitTypeBranched<UnitType::branch_t::NAVAL> const&, unlock_level_t);

bool CountryInstance::modify_unit_type_unlock(UnitType const& unit_type, unlock_level_t unlock_level_change) {
	using enum UnitType::branch_t;

	switch (unit_type.get_branch()) {
	case LAND:
		return modify_unit_type_unlock(static_cast<UnitTypeBranched<LAND> const&>(unit_type), unlock_level_change);
	case NAVAL:
		return modify_unit_type_unlock(static_cast<UnitTypeBranched<NAVAL> const&>(unit_type), unlock_level_change);
	default:
		Logger::error(
			"Attempted to change unlock level for unit type \"", unit_type.get_identifier(), "\" with invalid branch ",
			static_cast<uint32_t>(unit_type.get_branch()), " is unlocked for country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::unlock_unit_type(UnitType const& unit_type) {
	return modify_unit_type_unlock(unit_type, 1);
}

bool CountryInstance::is_unit_type_unlocked(UnitType const& unit_type) const {
	using enum UnitType::branch_t;

	switch (unit_type.get_branch()) {
	case LAND:
		return regiment_type_unlock_levels[static_cast<UnitTypeBranched<LAND> const&>(unit_type)] > 0;
	case NAVAL:
		return ship_type_unlock_levels[static_cast<UnitTypeBranched<NAVAL> const&>(unit_type)] > 0;
	default:
		Logger::error(
			"Attempted to check if unit type \"", unit_type.get_identifier(), "\" with invalid branch ",
			static_cast<uint32_t>(unit_type.get_branch()), " is unlocked for country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::modify_building_type_unlock(
	BuildingType const& building_type, unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	decltype(building_type_unlock_levels)::value_ref_type unlock_level = building_type_unlock_levels[building_type];

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for building type ", building_type.get_identifier(), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
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

bool CountryInstance::unlock_building_type(BuildingType const& building_type, GoodInstanceManager& good_instance_manager) {
	return modify_building_type_unlock(building_type, 1, good_instance_manager);
}

bool CountryInstance::is_building_type_unlocked(BuildingType const& building_type) const {
	return building_type_unlock_levels[building_type] > 0;
}

bool CountryInstance::modify_crime_unlock(Crime const& crime, unlock_level_t unlock_level_change) {
	decltype(crime_unlock_levels)::value_ref_type unlock_level = crime_unlock_levels[crime];

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for crime ", crime.get_identifier(), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(unlock_level + unlock_level_change)
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
	return crime_unlock_levels[crime] > 0;
}

bool CountryInstance::modify_gas_attack_unlock(unlock_level_t unlock_level_change) {
	// This catches subtracting below 0 or adding above the int types maximum value
	if (gas_attack_unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for gas attack in country ", get_identifier(),
			" to invalid value: current level = ", static_cast<int64_t>(gas_attack_unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(gas_attack_unlock_level + unlock_level_change)
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

bool CountryInstance::modify_gas_defence_unlock(unlock_level_t unlock_level_change) {
	// This catches subtracting below 0 or adding above the int types maximum value
	if (gas_defence_unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for gas defence in country ", get_identifier(),
			" to invalid value: current level = ", static_cast<int64_t>(gas_defence_unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(gas_defence_unlock_level + unlock_level_change)
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

bool CountryInstance::modify_unit_variant_unlock(unit_variant_t unit_variant, unlock_level_t unlock_level_change) {
	if (unit_variant < 1) {
		Logger::error("Trying to modify unlock level for default unit variant 0");
		return false;
	}

	if (unit_variant_unlock_levels.size() < unit_variant) {
		unit_variant_unlock_levels.resize(unit_variant);
	}

	unlock_level_t& unlock_level = unit_variant_unlock_levels[unit_variant - 1];

	bool ret = true;

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for unit variant ", static_cast<uint64_t>(unit_variant), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(unlock_level + unlock_level_change)
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

CountryInstance::unit_variant_t CountryInstance::get_max_unlocked_unit_variant() const {
	return unit_variant_unlock_levels.size();
}

bool CountryInstance::modify_technology_unlock(
	Technology const& technology, unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	decltype(technology_unlock_levels)::value_ref_type unlock_level = technology_unlock_levels[technology];

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for technology ", technology.get_identifier(), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(unlock_level + unlock_level_change)
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
		ret &= modify_building_type_unlock(*building, unlock_level_change, good_instance_manager);
	}

	return ret;
}

bool CountryInstance::set_technology_unlock_level(
	Technology const& technology, unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
) {
	const unlock_level_t unlock_level_change = unlock_level - technology_unlock_levels[technology];
	return unlock_level_change != 0 ? modify_technology_unlock(technology, unlock_level_change, good_instance_manager) : true;
}

bool CountryInstance::unlock_technology(Technology const& technology, GoodInstanceManager& good_instance_manager) {
	return modify_technology_unlock(technology, 1, good_instance_manager);
}

bool CountryInstance::is_technology_unlocked(Technology const& technology) const {
	return technology_unlock_levels[technology] > 0;
}

bool CountryInstance::modify_invention_unlock(
	Invention const& invention, unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	decltype(invention_unlock_levels)::value_ref_type unlock_level = invention_unlock_levels[invention];

	// This catches subtracting below 0 or adding above the int types maximum value
	if (unlock_level + unlock_level_change < 0) {
		Logger::error(
			"Attempted to change unlock level for invention ", invention.get_identifier(), " in country ",
			get_identifier(), " to invalid value: current level = ", static_cast<int64_t>(unlock_level), ", change = ",
			static_cast<int64_t>(unlock_level_change), ", invalid new value = ",
			static_cast<int64_t>(unlock_level + unlock_level_change)
		);
		return false;
	}

	const bool invention_was_unlocked = unlock_level > 0;
	unlock_level += unlock_level_change;
	if (invention_was_unlocked != (unlock_level > 0)) {
		if (invention_was_unlocked) {
			inventions_count--;
		} else {
			inventions_count++;
		}
	}

	bool ret = true;

	// TODO - handle invention.is_news()

	for (UnitType const* unit : invention.get_activated_units()) {
		ret &= modify_unit_type_unlock(*unit, unlock_level_change);
	}
	for (BuildingType const* building : invention.get_activated_buildings()) {
		ret &= modify_building_type_unlock(*building, unlock_level_change, good_instance_manager);
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
	Invention const& invention, unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
) {
	const unlock_level_t unlock_level_change = unlock_level - invention_unlock_levels[invention];
	return unlock_level_change != 0 ? modify_invention_unlock(invention, unlock_level_change, good_instance_manager) : true;
}

bool CountryInstance::unlock_invention(Invention const& invention, GoodInstanceManager& good_instance_manager) {
	return modify_invention_unlock(invention, 1, good_instance_manager);
}

bool CountryInstance::is_invention_unlocked(Invention const& invention) const {
	return invention_unlock_levels[invention] > 0;
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

fixed_point_t CountryInstance::calculate_research_cost(
	Technology const& technology, ModifierEffectCache const& modifier_effect_cache
) const {
	// TODO - what if research bonus is -100%? Divide by 0 -> infinite cost?
	return technology.get_cost() / (fixed_point_t::_1() + get_modifier_effect_value(
		*modifier_effect_cache.get_research_bonus_effects()[technology.get_area().get_folder()]
	));
}

fixed_point_t CountryInstance::get_research_progress() const {
	return current_research_cost > fixed_point_t::_0()
		? invested_research_points / current_research_cost
		: fixed_point_t::_0();
}

bool CountryInstance::can_research_tech(Technology const& technology, Date today) const {
	if (
		technology.get_year() > today.get_year() || !is_civilised() || is_technology_unlocked(technology) ||
		&technology == current_research
	) {
		return false;
	}

	const Technology::area_index_t index_in_area = technology.get_index_in_area();

	return index_in_area == 0 || is_technology_unlocked(*technology.get_area().get_technologies()[index_in_area - 1]);
}

void CountryInstance::start_research(Technology const& technology, InstanceManager const& instance_manager) {
	if (OV_unlikely(!can_research_tech(technology, instance_manager.get_today()))) {
		Logger::warning(
			"Attemping to start research for country \"", get_identifier(), "\" on technology \"",
			technology.get_identifier(), "\" - cannot research this tech!"
		);
		return;
	}

	current_research = &technology;
	invested_research_points = fixed_point_t::_0();

	_update_current_tech(instance_manager);
}

void CountryInstance::apply_foreign_investments(
	fixed_point_map_t<CountryDefinition const*> const& investments, CountryInstanceManager const& country_instance_manager
) {
	for (auto const& [country, money_invested] : investments) {
		foreign_investments[&country_instance_manager.get_country_instance_from_definition(*country)] = money_invested;
	}
}

bool CountryInstance::apply_history_to_country(CountryHistoryEntry const& entry, InstanceManager& instance_manager) {
	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
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
	ret &= upper_house.copy(entry.get_upper_house());
	if (entry.get_capital()) {
		capital = &instance_manager.get_map_instance().get_province_instance_from_definition(**entry.get_capital());
	}
	set_optional(government_type, entry.get_government_type());
	set_optional(plurality, entry.get_plurality());
	set_optional(national_value, entry.get_national_value());
	if (entry.is_civilised()) {
		country_status = *entry.is_civilised() ? COUNTRY_STATUS_CIVILISED : COUNTRY_STATUS_UNCIVILISED;
	}
	set_optional(prestige, entry.get_prestige());
	for (Reform const* reform : entry.get_reforms()) {
		ret &= add_reform(*reform);
	}
	set_optional(tech_school, entry.get_tech_school());
	constexpr auto set_bool_map_to_indexed_map =
		[]<typename T>(IndexedMap<T, bool>& target, ordered_map<T const*, bool> source) {
			for (auto const& [key, value] : source) {
				target[*key] = value;
			}
		};

	GoodInstanceManager& good_instance_manager = instance_manager.get_good_instance_manager();

	for (auto const& [technology, level] : entry.get_technologies()) {
		ret &= set_technology_unlock_level(*technology, level, good_instance_manager);
	}
	for (auto const& [invention, activated] : entry.get_inventions()) {
		ret &= set_invention_unlock_level(*invention, activated ? 1 : 0, good_instance_manager);
	}

	apply_foreign_investments(entry.get_foreign_investment(), instance_manager.get_country_instance_manager());

	set_optional(releasable_vassal, entry.is_releasable_vassal());

	// TODO - entry.get_colonial_points();

	ret &= apply_flag_map(entry.get_country_flags(), true);
	ret &= instance_manager.get_global_flags().apply_flag_map(entry.get_global_flags(), true);
	government_flag_overrides.write_non_empty_values(entry.get_government_flag_overrides());
	for (Decision const* decision : entry.get_decisions()) {
		// TODO - take decision
	}

	return ret;
}

void CountryInstance::_update_production(DefineManager const& define_manager) {
	// Calculate industrial power from states and foreign investments
	industrial_power = 0;
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
				define_manager.get_country_defines().get_country_investment_industrial_score_factor(),
				fixed_point_t::_100()
			);

			if (investment_industrial_power != 0) {
				industrial_power += investment_industrial_power;
				industrial_power_from_investments.emplace_back(country, investment_industrial_power);
			}
		}
	}

	std::sort(
		industrial_power_from_states.begin(), industrial_power_from_states.end(),
		[](auto const& a, auto const& b) -> bool { return a.second > b.second; }
	);
	std::sort(
		industrial_power_from_investments.begin(), industrial_power_from_investments.end(),
		[](auto const& a, auto const& b) -> bool { return a.second > b.second; }
	);
}

static inline constexpr fixed_point_t nonzero_or_one(fixed_point_t const& value) {
	return value == fixed_point_t::_0() ? fixed_point_t::_1() : value;
}

void CountryInstance::_update_budget(ModifierEffectCache const& modifier_effect_cache) {
	const fixed_point_t min_tax = get_modifier_effect_value(*modifier_effect_cache.get_min_tax());
	const fixed_point_t max_tax = nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_tax()));

	for (SliderValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
		tax_rate_slider_value.set_bounds(min_tax, max_tax);
	}

	// Education and administration spending sliders have no min/max modifiers

	social_spending_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_social_spending()),
		nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_social_spending()))
	);
	military_spending_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_military_spending()),
		nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_military_spending()))
	);
	tariff_rate_slider_value.set_bounds(
		get_modifier_effect_value(*modifier_effect_cache.get_min_tariff()),
		get_modifier_effect_value(*modifier_effect_cache.get_max_tariff())
	);

	// TODO - make sure we properly update everything dependent on these sliders' values,
	// as they might change if their sliders' bounds shrink past their previous values.
}

void CountryInstance::_update_current_tech(InstanceManager const& instance_manager) {
	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();

	current_research_cost = calculate_research_cost(
		*current_research, definition_manager.get_modifier_manager().get_modifier_effect_cache()
	);

	if (daily_research_points > fixed_point_t::_0()) {
		expected_research_completion_date = instance_manager.get_today() + static_cast<Timespan>(
			((current_research_cost - invested_research_points) / daily_research_points).ceil()
		);
	} else {
		expected_research_completion_date = definition_manager.get_define_manager().get_end_date();
	}
}

void CountryInstance::_update_technology(InstanceManager const& instance_manager) {
	if (research_point_stockpile < 0) {
		research_point_stockpile = 0;
	}

	ModifierEffectCache const& modifier_effect_cache =
		instance_manager.get_definition_manager().get_modifier_manager().get_modifier_effect_cache();

	daily_research_points += get_modifier_effect_value(*modifier_effect_cache.get_research_points());
	daily_research_points *= fixed_point_t::_1() +
		get_modifier_effect_value(*modifier_effect_cache.get_research_points_modifier()) +
		get_modifier_effect_value(*modifier_effect_cache.get_increase_research());

	if (daily_research_points < fixed_point_t::_0()) {
		daily_research_points = fixed_point_t::_0();
	}

	if (current_research != nullptr) {
		_update_current_tech(instance_manager);
	}
}

void CountryInstance::_update_politics() {

}

void CountryInstance::_update_population() {
	total_population = 0;
	national_literacy = 0;
	national_consciousness = 0;
	national_militancy = 0;

	population_by_strata.clear();
	militancy_by_strata.clear();
	life_needs_fulfilled_by_strata.clear();
	everyday_needs_fulfilled_by_strata.clear();
	luxury_needs_fulfilled_by_strata.clear();

	pop_type_distribution.clear();
	ideology_distribution.clear();
	issue_distribution.clear();
	vote_distribution.clear();
	culture_distribution.clear();
	religion_distribution.clear();

	for (State const* state : states) {
		total_population += state->get_total_population();

		// TODO - change casting if pop_size_t changes type
		const fixed_point_t state_population = fixed_point_t::parse(state->get_total_population());
		national_literacy += state->get_average_literacy() * state_population;
		national_consciousness += state->get_average_consciousness() * state_population;
		national_militancy += state->get_average_militancy() * state_population;

		population_by_strata += state->get_population_by_strata();
		militancy_by_strata.mul_add(state->get_militancy_by_strata(), state->get_population_by_strata());
		life_needs_fulfilled_by_strata.mul_add(
			state->get_life_needs_fulfilled_by_strata(), state->get_population_by_strata()
		);
		everyday_needs_fulfilled_by_strata.mul_add(
			state->get_everyday_needs_fulfilled_by_strata(), state->get_population_by_strata()
		);
		luxury_needs_fulfilled_by_strata.mul_add(
			state->get_luxury_needs_fulfilled_by_strata(), state->get_population_by_strata()
		);

		pop_type_distribution += state->get_pop_type_distribution();
		ideology_distribution += state->get_ideology_distribution();
		issue_distribution += state->get_issue_distribution();
		vote_distribution += state->get_vote_distribution();
		culture_distribution += state->get_culture_distribution();
		religion_distribution += state->get_religion_distribution();
	}

	if (total_population > 0) {
		national_literacy /= total_population;
		national_consciousness /= total_population;
		national_militancy /= total_population;

		militancy_by_strata /= population_by_strata;
		life_needs_fulfilled_by_strata /= population_by_strata;
		everyday_needs_fulfilled_by_strata /= population_by_strata;
		luxury_needs_fulfilled_by_strata /= population_by_strata;
	}

	daily_research_points = fixed_point_t::_0();
	monthly_leadership_points = fixed_point_t::_0();
	research_points_from_pop_types.clear();
	leadership_points_from_pop_types.clear();

	for (auto const& [pop_type, pop_size] : pop_type_distribution) {
		if (pop_type.get_research_leadership_optimum() > fixed_point_t::_0() && pop_size > 0) {
			const fixed_point_t factor = std::min(
				pop_size / (total_population * pop_type.get_research_leadership_optimum()), fixed_point_t::_1()
			);

			if (pop_type.get_research_points() != fixed_point_t::_0()) {
				const fixed_point_t research_points = pop_type.get_research_points() * factor;
				research_points_from_pop_types[&pop_type] = research_points;
				daily_research_points += research_points;
			}

			if (pop_type.get_leadership_points() != fixed_point_t::_0()) {
				const fixed_point_t leadership_points = pop_type.get_leadership_points() * factor;
				leadership_points_from_pop_types[&pop_type] = leadership_points;
				monthly_leadership_points = leadership_points;
			}
		}
	}

	// TODO - update national focus capacity
}

void CountryInstance::_update_trade() {
	// TODO - update total amount of each good exported and imported
}

void CountryInstance::_update_diplomacy() {
	// TODO - add prestige from modifiers
	// TODO - update diplomatic points and colonial power
}

void CountryInstance::_update_military(
	DefineManager const& define_manager, UnitTypeManager const& unit_type_manager,
	ModifierEffectCache const& modifier_effect_cache
) {
	MilitaryDefines const& military_defines = define_manager.get_military_defines();

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

	max_supported_regiment_count = 0;
	for (State const* state : states) {
		max_supported_regiment_count += state->get_max_supported_regiments();
	}

	supply_consumption = fixed_point_t::_1() + get_modifier_effect_value(*modifier_effect_cache.get_supply_consumption());

	const size_t regular_army_size = std::min(4 * deployed_non_mobilised_regiments, max_supported_regiment_count);

	fixed_point_t sum_of_regiment_type_stats = 0;
	for (RegimentType const& regiment_type : unit_type_manager.get_regiment_types()) {
		// TODO - apply country/tech modifiers to regiment stats
		sum_of_regiment_type_stats += (
			regiment_type.get_attack() + regiment_type.get_defence() /*+ land_attack_modifier + land_defense_modifier*/
		) * regiment_type.get_discipline();
	}

	military_power_from_land = supply_consumption * fixed_point_t::parse(regular_army_size) * sum_of_regiment_type_stats
		/ fixed_point_t::parse(7 * (1 + unit_type_manager.get_regiment_type_count()));

	if (disarmed) {
		military_power_from_land *= define_manager.get_diplomacy_defines().get_disarmed_penalty();
	}

	military_power_from_sea = 0;
	for (NavyInstance const* navy : navies) {
		for (ShipInstance const* ship : navy->get_ship_instances()) {
			ShipType const& ship_type = ship->get_ship_type();

			if (ship_type.is_capital()) {

				// TODO - include gun power and hull modifiers + naval attack and defense modifiers

				military_power_from_sea += (ship_type.get_gun_power() /*+ naval_attack_modifier*/)
					* (ship_type.get_hull() /* + naval_defense_modifier*/);
			}
		}
	}
	military_power_from_sea /= 250;

	military_power_from_leaders = fixed_point_t::parse(std::min(get_leader_count(), deployed_non_mobilised_regiments));

	military_power = military_power_from_land + military_power_from_sea + military_power_from_leaders;

	// Mobilisation calculations
	mobilisation_impact = get_modifier_effect_value(*modifier_effect_cache.get_mobilization_impact());
	mobilisation_economy_impact = get_modifier_effect_value(*modifier_effect_cache.get_mobilisation_economy_impact());

	// TODO - use country_defines.get_min_mobilize_limit(); (wiki: "lowest maximum of brigades you can mobilize. (by default 3)")

	mobilisation_max_regiment_count =
		((fixed_point_t::_1() + mobilisation_impact) * fixed_point_t::parse(regiment_count)).to_int64_t();

	mobilisation_potential_regiment_count = 0; // TODO - calculate max regiments from poor citizens
	if (mobilisation_potential_regiment_count > mobilisation_max_regiment_count) {
		mobilisation_potential_regiment_count = mobilisation_max_regiment_count;
	}

	// Limit max war exhaustion to non-negative values. This technically diverges from the base game where
	// max war exhaustion can be negative, but in such cases the war exhaustion clamping behaviour is
	// very buggy and regardless it doesn't seem to make any modifier effect contributions.
	war_exhaustion_max = std::max(
		get_modifier_effect_value(*modifier_effect_cache.get_max_war_exhaustion()), fixed_point_t::_0()
	);

	organisation_regain = fixed_point_t::_1() +
		get_modifier_effect_value(*modifier_effect_cache.get_org_regain()) +
		get_modifier_effect_value(*modifier_effect_cache.get_morale_global());

	land_organisation = fixed_point_t::_1() + get_modifier_effect_value(*modifier_effect_cache.get_land_organisation());
	naval_organisation = fixed_point_t::_1() + get_modifier_effect_value(*modifier_effect_cache.get_naval_organisation());

	land_unit_start_experience = get_modifier_effect_value(*modifier_effect_cache.get_regular_experience_level());
	naval_unit_start_experience = land_unit_start_experience;
	land_unit_start_experience += get_modifier_effect_value(*modifier_effect_cache.get_land_unit_start_experience());
	naval_unit_start_experience += get_modifier_effect_value(*modifier_effect_cache.get_naval_unit_start_experience());

	recruit_time = fixed_point_t::_1() + get_modifier_effect_value(*modifier_effect_cache.get_unit_recruitment_time());
	combat_width = fixed_point_t::parse(military_defines.get_base_combat_width()) +
		get_modifier_effect_value(*modifier_effect_cache.get_combat_width_additive());
	dig_in_cap = get_modifier_effect_value(*modifier_effect_cache.get_dig_in_cap());
	military_tactics = military_defines.get_base_military_tactics() +
		get_modifier_effect_value(*modifier_effect_cache.get_military_tactics());

	if (leadership_point_stockpile < 0) {
		leadership_point_stockpile = 0;
	}
	create_leader_count = leadership_point_stockpile / military_defines.get_leader_recruit_cost();

	monthly_leadership_points += get_modifier_effect_value(*modifier_effect_cache.get_leadership());
	monthly_leadership_points *= fixed_point_t::_1() +
		get_modifier_effect_value(*modifier_effect_cache.get_leadership_modifier());

	if (monthly_leadership_points < fixed_point_t::_0()) {
		monthly_leadership_points = fixed_point_t::_0();
	}

	// TODO - update max_ship_supply
}

bool CountryInstance::update_rule_set() {
	rule_set.clear();

	if (ruling_party != nullptr) {
		for (Issue const* issue : ruling_party->get_policies().get_values()) {
			if (issue != nullptr) {
				rule_set |= issue->get_rules();
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
	modifier_sum.add_modifier(static_modifier_cache.get_infamy(), infamy);
	modifier_sum.add_modifier(static_modifier_cache.get_literacy(), national_literacy);
	modifier_sum.add_modifier(static_modifier_cache.get_plurality(), plurality);
	modifier_sum.add_modifier(is_at_war() ? static_modifier_cache.get_war() : static_modifier_cache.get_peace());
	// TODO - difficulty modifiers, debt_default_to, bad_debtor, generalised_debt_default,
	//        total_occupation, total_blockaded, in_bankruptcy

	// TODO - handle triggered modifiers

	if (ruling_party != nullptr) {
		for (Issue const* issue : ruling_party->get_policies().get_values()) {
			// The ruling party's issues here could be null as they're stored in an IndexedMap which has
			// values for every IssueGroup regardless of whether or not they have a policy set.
			if (issue != nullptr) {
				modifier_sum.add_modifier(*issue);
			}
		}
	}

	for (Reform const* reform : reforms.get_values()) {
		// The country's reforms here could be null as they're stored in an IndexedMap which has
		// values for every ReformGroup regardless of whether or not they have a reform set.
		if (reform != nullptr) {
			modifier_sum.add_modifier(*reform);
		}
	}

	if (tech_school != nullptr) {
		modifier_sum.add_modifier(*tech_school);
	}

	for (Technology const& technology : *technology_unlock_levels.get_keys()) {
		if (is_technology_unlocked(technology)) {
			modifier_sum.add_modifier(technology);
		}
	}

	for (Invention const& invention : *invention_unlock_levels.get_keys()) {
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

	if (national_value != nullptr) {
		modifier_sum.add_modifier(*national_value);
	}

	// TODO - calculate stats for each unit type (locked and unlocked)
}

void CountryInstance::contribute_province_modifier_sum(ModifierSum const& province_modifier_sum) {
	modifier_sum.add_modifier_sum(province_modifier_sum);
}

fixed_point_t CountryInstance::get_modifier_effect_value(ModifierEffect const& effect) const {
	return modifier_sum.get_modifier_effect_value(effect);
}

void CountryInstance::update_gamestate(InstanceManager& instance_manager) {
	if (!is_civilised()) {
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

	MapInstance& map_instance = instance_manager.get_map_instance();

	neighbouring_countries.clear();
	for (ProvinceInstance const* province : owned_provinces) {
		for (ProvinceDefinition::adjacency_t const& adjacency : province->get_province_definition().get_adjacencies()) {
			// TODO - should we limit based on adjacency type? Straits and impassable still work in game,
			// and water provinces don't have an owner so they'll get caught by the later checks anyway.
			CountryInstance* neighbour =
				map_instance.get_province_instance_from_definition(*adjacency.get_to()).get_owner();
			if (neighbour != nullptr && neighbour != this) {
				neighbouring_countries.insert(neighbour);
			}
		}
	}

	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();
	DefineManager const& define_manager = definition_manager.get_define_manager();
	ModifierEffectCache const& modifier_effect_cache = definition_manager.get_modifier_manager().get_modifier_effect_cache();

	// Order of updates might need to be changed/functions split up to account for dependencies
	// Updates population stats (including research and leadership points from pops)
	_update_population();
	// Calculates industrial power
	_update_production(define_manager);
	// Calculates daily research points and predicts research completion date
	_update_technology(instance_manager);
	// Calculates national military modifiers, army and navy stats, daily leadership points
	_update_military(
		define_manager,
		definition_manager.get_military_manager().get_unit_type_manager(),
		modifier_effect_cache
	);
	_update_budget(modifier_effect_cache);

	// These don't do anything yet
	_update_politics();
	_update_trade();
	_update_diplomacy();

	total_score = prestige + industrial_power + military_power;

	const CountryDefinition::government_colour_map_t::const_iterator it =
		country_definition->get_alternative_colours().find(government_type);

	if (it != country_definition->get_alternative_colours().end()) {
		colour = it->second;
	} else {
		colour = country_definition->get_colour();
	}

	if (government_type != nullptr) {
		flag_government_type = government_flag_overrides[*government_type];

		if (flag_government_type == nullptr) {
			flag_government_type = government_type;
		}
	} else {
		flag_government_type = nullptr;
	}
}

void CountryInstance::country_reset_before_tick() {
	for (auto pair : goods_data) {
		pair.second.clear_daily_recorded_data();
	}
}

void CountryInstance::country_tick(InstanceManager& instance_manager) {
	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();
	DefineManager const& define_manager = definition_manager.get_define_manager();

	// Gain daily research points
	research_point_stockpile += daily_research_points;

	if (current_research != nullptr) {
		const fixed_point_t research_points_spent = std::min(
			std::min(research_point_stockpile, current_research_cost - invested_research_points),
			define_manager.get_economy_defines().get_max_daily_research()
		);

		research_point_stockpile -= research_points_spent;
		invested_research_points += research_points_spent;

		if (invested_research_points >= current_research_cost) {
			unlock_technology(*current_research, instance_manager.get_good_instance_manager());
			current_research = nullptr;
			invested_research_points = fixed_point_t::_0();
			current_research_cost = fixed_point_t::_0();
		}
	}

	// Apply maximum research point stockpile limit
	const fixed_point_t max_research_point_stockpile = is_civilised()
		? daily_research_points * static_cast<int32_t>(Date::DAYS_IN_YEAR)
		: define_manager.get_country_defines().get_max_research_points();
	if (research_point_stockpile > max_research_point_stockpile) {
		research_point_stockpile = max_research_point_stockpile;
	}

	// Gain monthly leadership points
	if (instance_manager.get_today().is_month_start()) {
		leadership_point_stockpile += monthly_leadership_points;
	}

	// TODO - auto create and auto assign leaders

	// Apply maximum leadership point stockpile limit
	const fixed_point_t max_leadership_point_stockpile =
		define_manager.get_military_defines().get_max_leadership_point_stockpile();
	if (leadership_point_stockpile > max_leadership_point_stockpile) {
		leadership_point_stockpile = max_leadership_point_stockpile;
	}

	//TODO add gold income from national money goods production
	//income_from_gold = country_defines.gold_to_cash_rate * sum (total_good_production * good.get_base_price()) for each good with is_money=true
}

CountryInstance::good_data_t::good_data_t()
	: lock { std::make_unique<std::mutex>() }
	{ }

void CountryInstance::good_data_t::clear_daily_recorded_data() {
	const std::lock_guard<std::mutex> lock_guard { *lock };
	stockpile_change_yesterday
		= exported_amount
		= government_needs
		= army_needs
		= navy_needs
		= overseas_maintenance
		= factory_demand
		= pop_demand
		= available_amount
		= fixed_point_t::_0();
	need_consumption_per_pop_type.clear();
	input_consumption_per_production_type.clear();
	production_per_production_type.clear();
}

void CountryInstance::report_pop_need_consumption(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
	good_data.need_consumption_per_pop_type[&pop_type] += quantity;
}
void CountryInstance::report_pop_need_demand(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
	good_data.pop_demand += quantity;
}
void CountryInstance::report_input_consumption(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
	good_data.input_consumption_per_production_type[&production_type] += quantity;
}
void CountryInstance::report_input_demand(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	if (production_type.get_template_type() == ProductionType::template_type_t::ARTISAN) {
		switch (game_rules_manager.get_artisanal_input_demand_category()) {
			case demand_category::FactoryNeeds: break;
			case demand_category::PopNeeds: {
				good_data_t& good_data = get_good_data(good);
				const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
				good_data.pop_demand += quantity;
				return;
			}
			default: return; //demand_category::None
		}
	}

	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
	good_data.factory_demand += quantity;
}
void CountryInstance::report_output(ProductionType const& production_type, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(production_type.get_output_good());
	const std::lock_guard<std::mutex> lock_guard { *good_data.lock };
	good_data.production_per_production_type[&production_type] += quantity;
}

CountryInstance::good_data_t& CountryInstance::get_good_data(GoodInstance const& good_instance) {
	return goods_data[good_instance];
}
CountryInstance::good_data_t const& CountryInstance::get_good_data(GoodInstance const& good_instance) const {
	return goods_data[good_instance];
}
CountryInstance::good_data_t& CountryInstance::get_good_data(GoodDefinition const& good_definition) {
	return goods_data[good_definition.get_index()];
}
CountryInstance::good_data_t const& CountryInstance::get_good_data(GoodDefinition const& good_definition) const {
	return goods_data[good_definition.get_index()];
}

void CountryInstanceManager::update_rankings(Date today, DefineManager const& define_manager) {
	total_ranking.clear();

	for (CountryInstance& country : country_instances.get_items()) {
		if (country.exists()) {
			total_ranking.push_back(&country);
		}
	}

	prestige_ranking = total_ranking;
	industrial_power_ranking = total_ranking;
	military_power_ranking = total_ranking;

	std::sort(
		total_ranking.begin(), total_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			const bool a_civilised = a->is_civilised();
			const bool b_civilised = b->is_civilised();
			return a_civilised != b_civilised ? a_civilised : a->get_total_score() > b->get_total_score();
		}
	);
	std::sort(
		prestige_ranking.begin(), prestige_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_prestige() > b->get_prestige();
		}
	);
	std::sort(
		industrial_power_ranking.begin(), industrial_power_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_industrial_power() > b->get_industrial_power();
		}
	);
	std::sort(
		military_power_ranking.begin(), military_power_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_military_power() > b->get_military_power();
		}
	);

	for (size_t index = 0; index < total_ranking.size(); ++index) {
		const size_t rank = index + 1;
		total_ranking[index]->total_rank = rank;
		prestige_ranking[index]->prestige_rank = rank;
		industrial_power_ranking[index]->industrial_rank = rank;
		military_power_ranking[index]->military_rank = rank;
	}

	CountryDefines const& country_defines = define_manager.get_country_defines();

	const size_t max_great_power_rank = country_defines.get_great_power_rank();
	const size_t max_secondary_power_rank = country_defines.get_secondary_power_rank();
	const Timespan lose_great_power_grace_days = country_defines.get_lose_great_power_grace_days();

	// Demote great powers who have been below the max great power rank for longer than the demotion grace period and
	// remove them from the list. We don't just demote them all and clear the list as when rebuilding we'd need to look
	// ahead for countries below the max great power rank but still within the demotion grace period.
	for (CountryInstance* great_power : great_powers) {
		if (great_power->get_total_rank() > max_great_power_rank && great_power->get_lose_great_power_date() < today) {
			great_power->country_status = COUNTRY_STATUS_CIVILISED;
		}
	}
	std::erase_if(great_powers, [](CountryInstance const* country) -> bool {
		return country->get_country_status() != COUNTRY_STATUS_GREAT_POWER;
	});

	// Demote all secondary powers and clear the list. We will rebuilt the whole list from scratch, so there's no need to
	// keep countries which are still above the max secondary power rank (they might become great powers instead anyway).
	for (CountryInstance* secondary_power : secondary_powers) {
		secondary_power->country_status = COUNTRY_STATUS_CIVILISED;
	}
	secondary_powers.clear();

	// Calculate the maximum number of countries eligible for great or secondary power status. This accounts for the
	// possibility of the max secondary power rank being higher than the max great power rank or both being zero, just
	// in case someone wants to experiment with only having secondary powers when some great power slots are filled by
	// countries in the demotion grace period, or having no great or secondary powers at all.
	const size_t max_power_index = std::clamp(max_secondary_power_rank, max_great_power_rank, total_ranking.size());

	for (size_t index = 0; index < max_power_index; index++) {
		CountryInstance* country = total_ranking[index];

		if (!country->is_civilised()) {
			// All further countries are civilised and so ineligible for great or secondary power status.
			break;
		}

		if (country->is_great_power()) {
			// The country already has great power status and is in the great powers list.
			continue;
		}

		if (great_powers.size() < max_great_power_rank && country->get_total_rank() <= max_great_power_rank) {
			// The country is eligible for great power status and there are still slots available,
			// so it is promoted and added to the list.
			country->country_status = COUNTRY_STATUS_GREAT_POWER;
			great_powers.push_back(country);
		} else if (country->get_total_rank() <= max_secondary_power_rank) {
			// The country is eligible for secondary power status and so is promoted and added to the list.
			country->country_status = COUNTRY_STATUS_SECONDARY_POWER;
			secondary_powers.push_back(country);
		}
	}

	// Sort the great powers list by total rank, as pre-existing great powers may have changed rank order and new great
	// powers will have been added to the end of the list regardless of rank.
	std::sort(great_powers.begin(), great_powers.end(), [](CountryInstance const* a, CountryInstance const* b) -> bool {
		return a->get_total_rank() < b->get_total_rank();
	});

	// Update the lose great power date for all great powers which are above the max great power rank.
	const Date new_lose_great_power_date = today + lose_great_power_grace_days;
	for (CountryInstance* great_power : great_powers) {
		if (great_power->get_total_rank() <= max_great_power_rank) {
			great_power->lose_great_power_date = new_lose_great_power_date;
		}
	}
}

CountryInstanceManager::CountryInstanceManager(CountryDefinitionManager const& new_country_definition_manager)
  : country_definition_manager { new_country_definition_manager },
	country_definition_to_instance_map { &new_country_definition_manager.get_country_definitions() } {}

CountryInstance& CountryInstanceManager::get_country_instance_from_definition(CountryDefinition const& country) {
	return *country_definition_to_instance_map[country];
}

CountryInstance const& CountryInstanceManager::get_country_instance_from_definition(CountryDefinition const& country) const {
	return *country_definition_to_instance_map[country];
}

bool CountryInstanceManager::generate_country_instances(
	decltype(CountryInstance::building_type_unlock_levels)::keys_type const& building_type_keys,
	decltype(CountryInstance::technology_unlock_levels)::keys_type const& technology_keys,
	decltype(CountryInstance::invention_unlock_levels)::keys_type const& invention_keys,
	decltype(CountryInstance::upper_house)::keys_type const& ideology_keys,
	decltype(CountryInstance::reforms)::keys_type const& reform_keys,
	decltype(CountryInstance::government_flag_overrides)::keys_type const& government_type_keys,
	decltype(CountryInstance::crime_unlock_levels)::keys_type const& crime_keys,
	decltype(CountryInstance::pop_type_distribution)::keys_type const& pop_type_keys,
	decltype(CountryInstance::goods_data)::keys_type const& good_instances_keys,
	decltype(CountryInstance::regiment_type_unlock_levels)::keys_type const& regiment_type_unlock_levels_keys,
	decltype(CountryInstance::ship_type_unlock_levels)::keys_type const& ship_type_unlock_levels_keys,
	decltype(CountryInstance::tax_rate_slider_value_by_strata):: keys_type const& strata_keys,
	GameRulesManager const& game_rules_manager,
	GoodInstanceManager& good_instance_manager,
	EconomyDefines const& economy_defines
) {
	reserve_more(country_instances, country_definition_manager.get_country_definition_count());

	bool ret = true;

	for (CountryDefinition const& country_definition : country_definition_manager.get_country_definitions()) {
		if (country_instances.add_item({
			&country_definition,
			get_country_instance_count(),
			building_type_keys,
			technology_keys,
			invention_keys,
			ideology_keys,
			reform_keys,
			government_type_keys,
			crime_keys,
			pop_type_keys,
			good_instances_keys,
			regiment_type_unlock_levels_keys,
			ship_type_unlock_levels_keys,
			strata_keys,
			game_rules_manager,
			good_instance_manager,
			economy_defines
		})) {
			// We need to update the country's ModifierSum's source here as the country's address is finally stable
			// after changing between its constructor call and now due to being std::move'd into the registry.
			CountryInstance& country_instance = get_back_country_instance();
			country_instance.modifier_sum.set_this_source(&country_instance);

			country_definition_to_instance_map[country_definition] = &country_instance;
		} else {
			ret = false;
		}
	}

	return ret;
}

bool CountryInstanceManager::apply_history_to_countries(
	CountryHistoryManager const& history_manager, InstanceManager& instance_manager
) {
	bool ret = true;

	const Date today = instance_manager.get_today();
	UnitInstanceManager& unit_instance_manager = instance_manager.get_unit_instance_manager();
	MapInstance& map_instance = instance_manager.get_map_instance();

	for (CountryInstance& country_instance : country_instances.get_items()) {
		if (!country_instance.get_country_definition()->is_dynamic_tag()) {
			CountryHistoryMap const* history_map =
				history_manager.get_country_history(country_instance.get_country_definition());

			if (history_map != nullptr) {
				static constexpr fixed_point_t DEFAULT_STATE_CULTURE_LITERACY = fixed_point_t::_0_50();
				CountryHistoryEntry const* oob_history_entry = nullptr;
				std::optional<fixed_point_t> state_culture_consciousness;
				std::optional<fixed_point_t> nonstate_culture_consciousness;
				fixed_point_t state_culture_literacy = DEFAULT_STATE_CULTURE_LITERACY;
				std::optional<fixed_point_t> nonstate_culture_literacy;

				for (auto const& [entry_date, entry] : history_map->get_entries()) {
					if (entry_date <= today) {
						ret &= country_instance.apply_history_to_country(*entry, instance_manager);

						if (entry->get_inital_oob().has_value()) {
							oob_history_entry = entry.get();
						}
						if (entry->get_consciousness().has_value()) {
							state_culture_consciousness = entry->get_consciousness();
						}
						if (entry->get_nonstate_consciousness().has_value()) {
							nonstate_culture_consciousness = entry->get_nonstate_consciousness();
						}
						if (entry->get_literacy().has_value()) {
							state_culture_literacy = *entry->get_literacy();
						}
						if (entry->get_nonstate_culture_literacy().has_value()) {
							nonstate_culture_literacy = entry->get_nonstate_culture_literacy();
						}
					} else {
						// All foreign investments are applied regardless of the bookmark's date
						country_instance.apply_foreign_investments(entry->get_foreign_investment(), *this);
					}
				}

				if (oob_history_entry != nullptr) {
					ret &= unit_instance_manager.generate_deployment(
						map_instance, country_instance, *oob_history_entry->get_inital_oob()
					);
				}

				// TODO - check if better to do "if"s then "for"s, so looping multiple times rather than having lots of
				// redundant "if" statements?
				for (ProvinceInstance* province : country_instance.get_owned_provinces()) {
					for (Pop& pop : province->get_mutable_pops()) {
						if (country_instance.is_primary_or_accepted_culture(pop.get_culture())) {
							pop.set_literacy(state_culture_literacy);

							if (state_culture_consciousness.has_value()) {
								pop.set_consciousness(*state_culture_consciousness);
							}
						} else {
							if (nonstate_culture_literacy.has_value()) {
								pop.set_literacy(*nonstate_culture_literacy);
							}

							if (nonstate_culture_consciousness.has_value()) {
								pop.set_consciousness(*nonstate_culture_consciousness);
							}
						}
					}
				}
			} else {
				Logger::error("Country ", country_instance.get_identifier(), " has no history!");
				ret = false;
			}
		}
	}

	return ret;
}

void CountryInstanceManager::update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache) {
	for (CountryInstance& country : country_instances.get_items()) {
		country.update_modifier_sum(today, static_modifier_cache);
	}
}

void CountryInstanceManager::update_gamestate(InstanceManager& instance_manager) {
	for (CountryInstance& country : country_instances.get_items()) {
		country.update_gamestate(instance_manager);
	}

	update_rankings(instance_manager.get_today(), instance_manager.get_definition_manager().get_define_manager());
}

void CountryInstanceManager::country_manager_reset_before_tick() {
	for (CountryInstance& country : country_instances.get_items()) {
		country.country_reset_before_tick();
	}
}

void CountryInstanceManager::country_manager_tick(InstanceManager& instance_manager) {
	for (CountryInstance& country : country_instances.get_items()) {
		country.country_tick(instance_manager);
	}
}
