#include "CountryInstance.hpp"

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/ClampedValue.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

using enum CountryInstance::country_status_t;

static constexpr colour_t ERROR_COLOUR = colour_t::from_integer(0xFF0000);

CountryInstance::CountryInstance(
	CountryDefinition const* new_country_definition,
	decltype(building_type_unlock_levels)::keys_span_type building_type_keys,
	decltype(technology_unlock_levels)::keys_span_type technology_keys,
	decltype(invention_unlock_levels)::keys_span_type invention_keys,
	decltype(reforms)::keys_span_type reform_keys,
	decltype(flag_overrides_by_government_type)::keys_span_type government_type_keys,
	decltype(crime_unlock_levels)::keys_span_type crime_keys,
	decltype(goods_data)::keys_span_type good_instances_keys,
	decltype(regiment_type_unlock_levels)::keys_span_type regiment_type_unlock_levels_keys,
	decltype(ship_type_unlock_levels)::keys_span_type ship_type_unlock_levels_keys,
	utility::forwardable_span<const Strata> strata_keys,
	utility::forwardable_span<const PopType> pop_type_keys,
	utility::forwardable_span<const Ideology> ideology_keys,
	GameRulesManager const* new_game_rules_manager,
	CountryRelationManager* new_country_relations_manager,
	SharedCountryValues* new_shared_country_values,
	GoodInstanceManager* new_good_instance_manager,
	CountryDefines const* new_country_defines,
	EconomyDefines const* new_economy_defines
) : FlagStrings { "country" },
	HasIndex { new_country_definition->get_index() },
	PopsAggregate { strata_keys, pop_type_keys, ideology_keys },
	/* Main attributes */
	country_definition { *new_country_definition },
	game_rules_manager { *new_game_rules_manager },
	country_relations_manager { *new_country_relations_manager },
	shared_country_values { *new_shared_country_values },
	country_defines { *new_country_defines },
	colour { ERROR_COLOUR },

	/* Production */
	building_type_unlock_levels { building_type_keys },

	/* Budget */
	taxable_income_by_pop_type { pop_type_keys },
	effective_tax_rate_by_strata {
		strata_keys,
		[this](Strata const& strata)->auto {
			return [this,&strata](DependencyTracker& tracker)->fixed_point_t {
				return tax_efficiency.get(tracker) * tax_rate_slider_value_by_strata.at(strata).get_value(tracker);
			};
		}
	},
	administration_salary_base_by_pop_type{
		pop_type_keys,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_administration_salary_base(tracker);
			};
		}
	},
	education_salary_base_by_pop_type{
		pop_type_keys,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_education_salary_base(tracker);
			};
		}
	},
	military_salary_base_by_pop_type{
		pop_type_keys,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_military_salary_base(tracker);
			};
		}
	},
	social_income_variant_base_by_pop_type{
		pop_type_keys,
		[this](PopType const& pop_type)->auto {
			return [this,&pop_type](DependencyTracker& tracker)->fixed_point_t {
				return corruption_cost_multiplier.get(tracker)
					* shared_country_values.get_shared_pop_type_values(pop_type)
						.get_social_income_variant_base(tracker);
			};
		}
	},
	tax_rate_slider_value_by_strata { strata_keys },

	/* Technology */
	technology_unlock_levels { technology_keys },
	invention_unlock_levels { invention_keys },

	/* Politics */
	upper_house_proportion_by_ideology { ideology_keys },
	reforms { reform_keys },
	flag_overrides_by_government_type { government_type_keys },
	crime_unlock_levels { crime_keys },

	/* Trade */
	goods_data { good_instances_keys },

	/* Diplomacy */

	/* Military */
	regiment_type_unlock_levels { regiment_type_unlock_levels_keys },
	ship_type_unlock_levels { ship_type_unlock_levels_keys },

	/* DerivedState */
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
		return current_research_cost_copy > fixed_point_t::_0
			? invested_research_points.get(tracker) / current_research_cost_copy
			: fixed_point_t::_0;
	}},
	desired_administrator_percentage{[this](DependencyTracker& tracker)->fixed_point_t {
		return country_defines.get_max_bureaucracy_percentage()
			+ total_administrative_multiplier.get(tracker) * country_defines.get_bureaucracy_percentage_increment();
	}},
	corruption_cost_multiplier{[this](DependencyTracker& tracker)->fixed_point_t {
		return 2 - administrative_efficiency_from_administrators.get(tracker);
	}},
	tariff_efficiency{[this](DependencyTracker& tracker)->fixed_point_t {
		return std::min(
			fixed_point_t::_1,
			administrative_efficiency_from_administrators.get(tracker) + country_defines.get_base_country_tax_efficiency()
		);
	}},
	effective_tariff_rate{[this](DependencyTracker& tracker)->fixed_point_t {
		return administrative_efficiency_from_administrators.get(tracker) * tariff_rate_slider_value.get_value(tracker);
	}},
	projected_administration_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return administration_spending_slider_value.get_value(tracker) * projected_administration_spending_unscaled_by_slider.get(tracker);
	}},
	projected_education_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return education_spending_slider_value.get_value(tracker) * projected_education_spending_unscaled_by_slider.get(tracker);
	}},
	projected_military_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return military_spending_slider_value.get_value(tracker) * projected_military_spending_unscaled_by_slider.get(tracker);
	}},
	projected_social_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return social_spending_slider_value.get_value(tracker) * projected_social_spending_unscaled_by_slider.get(tracker);
	}},
	projected_social_spending_unscaled_by_slider{[this](DependencyTracker& tracker)->fixed_point_t {
		return projected_pensions_spending_unscaled_by_slider.get(tracker)
			+ projected_unemployment_subsidies_spending_unscaled_by_slider.get(tracker);
	}},
	projected_import_subsidies{[this](DependencyTracker& tracker)->fixed_point_t {
		return has_import_subsidies.get(tracker)
			? -effective_tariff_rate.get(tracker) * get_yesterdays_import_value(tracker)
			: fixed_point_t::_0;
	}},
	projected_spending{[this](DependencyTracker& tracker)->fixed_point_t {
		return projected_administration_spending.get(tracker)
			+ projected_education_spending.get(tracker)
			+ projected_military_spending.get(tracker)
			+ projected_social_spending.get(tracker)
			+ projected_import_subsidies.get(tracker);
	}},
	has_import_subsidies{[this](DependencyTracker& tracker)->bool {
		return effective_tariff_rate.get(tracker) < 0;
	}} {
	modifier_sum.set_this_source(this);
	// Exclude PROVINCE (local) modifier effects from the country's modifier sum
	modifier_sum.set_this_excluded_targets(ModifierEffect::target_t::PROVINCE);

	// Some sliders need to have their max range limits temporarily set to 1 so they can start with a value of 0.5 or 1.0.
	// The range limits will be corrected on the first gamestate update, and the values will go to the closest valid point.
	for (ClampedValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
		tax_rate_slider_value.set_bounds(0, 1);
		tax_rate_slider_value.set_value(fixed_point_t::_0_50);
	}

	// army, navy and construction spending have minimums defined in EconomyDefines and always have an unmodified max (1.0).
	army_spending_slider_value.set_bounds(new_economy_defines->get_minimum_army_spending_slider_value(), 1);
	army_spending_slider_value.set_value(1);

	navy_spending_slider_value.set_bounds(new_economy_defines->get_minimum_navy_spending_slider_value(), 1);
	navy_spending_slider_value.set_value(1);

	construction_spending_slider_value.set_bounds(
		new_economy_defines->get_minimum_construction_spending_slider_value(), 1
	);
	construction_spending_slider_value.set_value(1);

	education_spending_slider_value.set_bounds(0, 1);
	education_spending_slider_value.set_value(fixed_point_t::_0_50);

	administration_spending_slider_value.set_bounds(0, 1);
	administration_spending_slider_value.set_value(fixed_point_t::_0_50);

	social_spending_slider_value.set_bounds(0, 1);
	social_spending_slider_value.set_value(1);

	military_spending_slider_value.set_bounds(0, 1);
	military_spending_slider_value.set_value(fixed_point_t::_0_50);

	tariff_rate_slider_value.set_bounds(0, 0);
	tariff_rate_slider_value.set_value(0);

	update_parties_for_votes(new_country_definition);

	for (BuildingType const& building_type : building_type_unlock_levels.get_keys()) {
		if (building_type.is_default_enabled()) {
			unlock_building_type(building_type, *new_good_instance_manager);
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
		Logger::warning("Attempting to add, '", country.get_identifier() ,"' to a sphere when it is already included in a sphere.");
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

fixed_point_t CountryInstance::get_taxable_income_by_strata(Strata const& strata) const {
	fixed_point_t running_total = 0;
	for (auto const& [pop_type, taxable_income] : taxable_income_by_pop_type) {
		if (pop_type.get_strata() == strata) {
			running_total += taxable_income;
		}
	}
	
	return running_total;
}
DerivedState<fixed_point_t>& CountryInstance::get_effective_tax_rate_by_strata(Strata const& strata) {
	return effective_tax_rate_by_strata.at(strata);
}

ReadOnlyClampedValue& CountryInstance::get_tax_rate_slider_value_by_strata(Strata const& strata) {
	return tax_rate_slider_value_by_strata.at(strata);
}
ReadOnlyClampedValue const& CountryInstance::get_tax_rate_slider_value_by_strata(Strata const& strata) const {
	return tax_rate_slider_value_by_strata.at(strata);
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

void CountryInstance::set_strata_tax_rate_slider_value(Strata const& strata, const fixed_point_t new_value) {
	tax_rate_slider_value_by_strata.at(strata).set_value(new_value);
}

void CountryInstance::set_army_spending_slider_value(const fixed_point_t new_value) {
	army_spending_slider_value.set_value(new_value);
}

void CountryInstance::set_navy_spending_slider_value(const fixed_point_t new_value) {
	navy_spending_slider_value.set_value(new_value);
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
		Logger::error(
			"Trying to add unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " to country ", get_identifier()
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
			Logger::error(
				"Trying to remove non-existent ", get_branched_unit_group_name(Branch), " \"",
				group.get_name(), "\" from country ", get_identifier()
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
		Logger::error(
			"Trying to remove unit group \"", group.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(group.get_branch()), " from country ", get_identifier()
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
		Logger::error(
			"Trying to add leader \"", leader.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(leader.get_branch()), " to country ", get_identifier()
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
		Logger::error(
			"Trying to remove leader \"", leader.get_name(), "\" with invalid branch ",
			static_cast<uint32_t>(leader.get_branch()), " from country ", get_identifier()
		);
		return false;
	}

	const typename memory::vector<LeaderInstance*>::const_iterator it = std::find(leaders->begin(), leaders->end(), &leader);

	if (it != leaders->end()) {
		leaders->erase(it);
		return true;
	} else {
		Logger::error(
			"Trying to remove non-existent ", get_branched_leader_name(leader.get_branch()), " \"",
			leader.get_name(), "\" from country ", get_identifier()
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
	using enum unit_branch_t;

	switch (unit_type.get_branch()) {
	case LAND:
		return regiment_type_unlock_levels.at(static_cast<UnitTypeBranched<LAND> const&>(unit_type)) > 0;
	case NAVAL:
		return ship_type_unlock_levels.at(static_cast<UnitTypeBranched<NAVAL> const&>(unit_type)) > 0;
	default:
		Logger::error(
			"Attempted to check if unit type \"", unit_type.get_identifier(), "\" with invalid branch ",
			static_cast<uint32_t>(unit_type.get_branch()), " is unlocked for country ", get_identifier()
		);
		return false;
	}
}

bool CountryInstance::modify_building_type_unlock(
	BuildingType const& building_type, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	technology_unlock_level_t& unlock_level = building_type_unlock_levels.at(building_type);

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
	return building_type_unlock_levels.at(building_type) > 0;
}

bool CountryInstance::modify_crime_unlock(Crime const& crime, technology_unlock_level_t unlock_level_change) {
	technology_unlock_level_t& unlock_level = crime_unlock_levels.at(crime);

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
	return crime_unlock_levels.at(crime) > 0;
}

bool CountryInstance::modify_gas_attack_unlock(technology_unlock_level_t unlock_level_change) {
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

bool CountryInstance::modify_gas_defence_unlock(technology_unlock_level_t unlock_level_change) {
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

bool CountryInstance::modify_unit_variant_unlock(unit_variant_t unit_variant, technology_unlock_level_t unlock_level_change) {
	if (unit_variant < 1) {
		Logger::error("Trying to modify unlock level for default unit variant 0");
		return false;
	}

	if (unit_variant_unlock_levels.size() < unit_variant) {
		unit_variant_unlock_levels.resize(unit_variant);
	}

	technology_unlock_level_t& unlock_level = unit_variant_unlock_levels[unit_variant - 1];

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

unit_variant_t CountryInstance::get_max_unlocked_unit_variant() const {
	return unit_variant_unlock_levels.size();
}

bool CountryInstance::modify_technology_unlock(
	Technology const& technology, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	technology_unlock_level_t& unlock_level = technology_unlock_levels.at(technology);

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
	Technology const& technology, technology_unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
) {
	const technology_unlock_level_t unlock_level_change = unlock_level - technology_unlock_levels.at(technology);
	return unlock_level_change != 0 ? modify_technology_unlock(technology, unlock_level_change, good_instance_manager) : true;
}

bool CountryInstance::unlock_technology(Technology const& technology, GoodInstanceManager& good_instance_manager) {
	return modify_technology_unlock(technology, 1, good_instance_manager);
}

bool CountryInstance::is_technology_unlocked(Technology const& technology) const {
	return technology_unlock_levels.at(technology) > 0;
}

bool CountryInstance::modify_invention_unlock(
	Invention const& invention, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
) {
	technology_unlock_level_t& unlock_level = invention_unlock_levels.at(invention);

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
	Invention const& invention, technology_unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
) {
	const technology_unlock_level_t unlock_level_change = unlock_level - invention_unlock_levels.at(invention);
	return unlock_level_change != 0 ? modify_invention_unlock(invention, unlock_level_change, good_instance_manager) : true;
}

bool CountryInstance::unlock_invention(Invention const& invention, GoodInstanceManager& good_instance_manager) {
	return modify_invention_unlock(invention, 1, good_instance_manager);
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

fixed_point_t CountryInstance::calculate_research_cost(
	Technology const& technology, ModifierEffectCache const& modifier_effect_cache
) const {
	// TODO - what if research bonus is -100%? Divide by 0 -> infinite cost?
	return technology.get_cost() / (fixed_point_t::_1 + get_modifier_effect_value(
		*modifier_effect_cache.get_research_bonus_effects(technology.get_area().get_folder())
	));
}

bool CountryInstance::can_research_tech(Technology const& technology, Date today) const {
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

void CountryInstance::start_research(Technology const& technology, InstanceManager const& instance_manager) {
	if (OV_unlikely(!can_research_tech(technology, instance_manager.get_today()))) {
		Logger::warning(
			"Attempting to start research for country \"", get_identifier(), "\" on technology \"",
			technology.get_identifier(), "\" - cannot research this tech!"
		);
		return;
	}

	current_research.set(&technology);
	invested_research_points.set(0);

	_update_current_tech(instance_manager);
}

void CountryInstance::apply_foreign_investments(
	fixed_point_map_t<CountryDefinition const*> const& investments, CountryInstanceManager const& country_instance_manager
) {
	for (auto const& [country, money_invested] : investments) {
		foreign_investments[&country_instance_manager.get_country_instance_by_definition(*country)] = money_invested;
	}
}

bool CountryInstance::apply_history_to_country(CountryHistoryEntry const& entry, InstanceManager& instance_manager) {
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
		capital = &instance_manager.get_map_instance().get_province_instance_by_definition(**entry.get_capital());
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
	flag_overrides_by_government_type.copy_values_from(entry.get_flag_overrides_by_government_type());
	for (Decision const* decision : entry.get_decisions()) {
		// TODO - take decision
	}

	return ret;
}

void CountryInstance::_update_production(DefineManager const& define_manager) {
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
				define_manager.get_country_defines().get_country_investment_industrial_score_factor(),
				100
			);

			if (investment_industrial_power != 0) {
				industrial_power += investment_industrial_power;
				industrial_power_from_investments.emplace_back(country, investment_industrial_power);
			}
		}
	}

	industrial_power.set(industrial_power_running_total);

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
	return value == fixed_point_t::_0 ? fixed_point_t::_1 : value;
}

void CountryInstance::_update_budget() {
	CountryDefines const& country_defines = shared_country_values.get_country_defines();
	ModifierEffectCache const& modifier_effect_cache = shared_country_values.get_modifier_effect_cache();

	const fixed_point_t min_tax = get_modifier_effect_value(*modifier_effect_cache.get_min_tax());
	const fixed_point_t max_tax = nonzero_or_one(get_modifier_effect_value(*modifier_effect_cache.get_max_tax()));

	for (ClampedValue& tax_rate_slider_value : tax_rate_slider_value_by_strata.get_values()) {
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

	tax_efficiency.set(
		country_defines.get_base_country_tax_efficiency()
		+ get_modifier_effect_value(*modifier_effect_cache.get_tax_efficiency())
		+ get_modifier_effect_value(*modifier_effect_cache.get_tax_eff()) / 100
	);

	/*
	In Victoria 2, administration efficiency is updated in the UI immediately.
	However the corruption_cost_multiplier is only updated after 2 ticks.

	OpenVic immediately updates both.
	*/

	pop_size_t total_non_colonial_population = 0;
	pop_size_t administrators = 0;
	for (State const* const state_ptr : states) {
		if (state_ptr == nullptr) {
			continue;
		}

		State const& state = *state_ptr;
		if (state.is_colonial_state()) {
			continue;
		}

		IndexedFlatMap<PopType, pop_size_t> const& state_population_by_type = state.get_population_by_type();

		for (auto const& [pop_type, size] : state_population_by_type) {
			if (pop_type.get_is_administrator()) {
				administrators += size;
			}
		}
		total_non_colonial_population += state.get_total_population();
	}

	if (total_non_colonial_population == 0) {
		administrative_efficiency_from_administrators.set(fixed_point_t::_1);
		administrator_percentage.set(fixed_point_t::_0);
	} else {		
		administrator_percentage.set(fixed_point_t::parse(administrators) / total_non_colonial_population);

		const fixed_point_t desired_administrators = desired_administrator_percentage.get_untracked() * total_non_colonial_population;
		const fixed_point_t administrative_efficiency_from_administrators_unclamped = std::min(
			fixed_point_t::mul_div(
				administrators,
				fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_administrative_efficiency()),
				desired_administrators
			)
			* (fixed_point_t::_1 + get_modifier_effect_value(*modifier_effect_cache.get_administrative_efficiency_modifier())),
			fixed_point_t::_1
		);

		administrative_efficiency_from_administrators.set(
			game_rules_manager.get_prevent_negative_administration_efficiency()
			? std::max(fixed_point_t::_0, administrative_efficiency_from_administrators_unclamped)
			: administrative_efficiency_from_administrators_unclamped
		);
	}

	fixed_point_t projected_administration_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_education_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_military_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_pensions_spending_unscaled_by_slider_running_total = 0;
	fixed_point_t projected_unemployment_subsidies_spending_unscaled_by_slider_running_total = 0;

	for (auto const& [pop_type, size] : get_population_by_type()) {
		projected_administration_spending_unscaled_by_slider_running_total += size * administration_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_education_spending_unscaled_by_slider_running_total += size * education_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_military_spending_unscaled_by_slider_running_total += size * military_salary_base_by_pop_type.at(pop_type).get_untracked();
		projected_pensions_spending_unscaled_by_slider_running_total += size * calculate_pensions_base(modifier_effect_cache, pop_type);
		projected_unemployment_subsidies_spending_unscaled_by_slider_running_total += get_unemployed_pops_by_type(pop_type)
			* calculate_unemployment_subsidies_base(modifier_effect_cache, pop_type);
	}

	projected_administration_spending_unscaled_by_slider.set(
		projected_administration_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_education_spending_unscaled_by_slider.set(
		projected_education_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_military_spending_unscaled_by_slider.set(
		projected_military_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_pensions_spending_unscaled_by_slider.set(
		projected_pensions_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
	projected_unemployment_subsidies_spending_unscaled_by_slider.set(
		projected_unemployment_subsidies_spending_unscaled_by_slider_running_total / Pop::size_denominator
	);
}

fixed_point_t CountryInstance::calculate_pensions_base(
	ModifierEffectCache const& modifier_effect_cache,
	PopType const& pop_type
) {
	return get_modifier_effect_value(*modifier_effect_cache.get_pension_level())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}
fixed_point_t CountryInstance::calculate_unemployment_subsidies_base(
	ModifierEffectCache const& modifier_effect_cache,
	PopType const& pop_type
) {
	return get_modifier_effect_value(*modifier_effect_cache.get_unemployment_benefit())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}
fixed_point_t CountryInstance::calculate_minimum_wage_base(
	ModifierEffectCache const& modifier_effect_cache,
	PopType const& pop_type
) {
	return get_modifier_effect_value(*modifier_effect_cache.get_minimum_wage())
		* social_income_variant_base_by_pop_type.at(pop_type).get_untracked();
}

void CountryInstance::_update_current_tech(InstanceManager const& instance_manager) {
	Technology const* current_research_copy = current_research.get_untracked();
	if (current_research_copy == nullptr) {
		return;
	}

	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();
	current_research_cost.set(calculate_research_cost(
		*current_research_copy, definition_manager.get_modifier_manager().get_modifier_effect_cache()
	));

	const fixed_point_t daily_research_points_copy = daily_research_points.get_untracked();
	if (daily_research_points_copy > fixed_point_t::_0) {
		expected_research_completion_date.set(
			instance_manager.get_today()
			+ static_cast<Timespan>(
				(
					(current_research_cost.get_untracked() - invested_research_points.get_untracked())
					/ daily_research_points_copy
				).ceil()
			)
		);
	} else {
		expected_research_completion_date.set(definition_manager.get_define_manager().get_end_date());
	}
}

void CountryInstance::_update_technology(InstanceManager const& instance_manager) {
	if (research_point_stockpile.get_untracked() < fixed_point_t::_0) {
		research_point_stockpile.set(0);
	}

	ModifierEffectCache const& modifier_effect_cache =
		instance_manager.get_definition_manager().get_modifier_manager().get_modifier_effect_cache();

	daily_research_points += get_modifier_effect_value(*modifier_effect_cache.get_research_points());
	daily_research_points *= fixed_point_t::_1 +
		get_modifier_effect_value(*modifier_effect_cache.get_research_points_modifier()) +
		get_modifier_effect_value(*modifier_effect_cache.get_increase_research());

	if (daily_research_points.get_untracked() < fixed_point_t::_0) {
		daily_research_points.set(0);
	}

	_update_current_tech(instance_manager);
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
		if (pop_type.get_research_leadership_optimum() > fixed_point_t::_0 && pop_size > 0) {
			const fixed_point_t factor = std::min(
				pop_size / (get_total_population() * pop_type.get_research_leadership_optimum()), fixed_point_t::_1
			);

			if (pop_type.get_research_points() != fixed_point_t::_0) {
				const fixed_point_t research_points = pop_type.get_research_points() * factor;
				research_points_from_pop_types[&pop_type] = research_points;
				daily_research_points += research_points;
			}

			if (pop_type.get_leadership_points() != fixed_point_t::_0) {
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
		military_power_from_land *= define_manager.get_diplomacy_defines().get_disarmed_penalty();
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
		((fixed_point_t::_1 + mobilisation_impact) * fixed_point_t::parse(regiment_count)).to_int64_t();

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
	monthly_leadership_points *= fixed_point_t::_1 +
		get_modifier_effect_value(*modifier_effect_cache.get_leadership_modifier());

	if (monthly_leadership_points < fixed_point_t::_0) {
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

void CountryInstance::update_gamestate(InstanceManager& instance_manager) {
	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();
	DefineManager const& define_manager = definition_manager.get_define_manager();
	ModifierEffectCache const& modifier_effect_cache = shared_country_values.get_modifier_effect_cache();

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

		if (owned_cores_controlled_proportion != fixed_point_t::_0) {
			owned_cores_controlled_proportion /= owned_core_province_count;
		}
	}

	MapInstance& map_instance = instance_manager.get_map_instance();

	occupied_provinces_proportion = 0;
	port_count = 0;
	neighbouring_countries.clear();

	Continent const* capital_continent = capital != nullptr ? capital->get_province_definition().get_continent() : nullptr;

	for (ProvinceInstance* province : owned_provinces) {
		ProvinceDefinition const& province_definition = province->get_province_definition();

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

	if (occupied_provinces_proportion != fixed_point_t::_0) {
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
	_update_production(define_manager);
	// Calculates daily research points and predicts research completion date
	_update_technology(instance_manager);
	// Calculates national military modifiers, army and navy stats, daily leadership points
	_update_military(
		define_manager,
		definition_manager.get_military_manager().get_unit_type_manager(),
		modifier_effect_cache
	);
	_update_budget();

	// These don't do anything yet
	_update_politics();
	_update_trade();
	_update_diplomacy();

	const CountryDefinition::government_colour_map_t::const_iterator it =
		country_definition.get_alternative_colours().find(government_type.get_untracked());

	if (it != country_definition.get_alternative_colours().end()) {
		colour = it.value();
	} else {
		colour = country_definition.get_colour();
	}
}

void CountryInstance::country_tick_before_map(InstanceManager& instance_manager) {
	//TODO AI sliders
	//TODO stockpile management

	// + reparations + war subsidies
	// + stockpile costs
	// + industrial subsidies
	// + loan interest

	fixed_point_t available_funds = cash_stockpile;
	fixed_point_t actual_import_subsidies;
	const fixed_point_t projected_administration_spending_copy = projected_administration_spending.get_untracked();
	const fixed_point_t projected_education_spending_copy = projected_education_spending.get_untracked();
	const fixed_point_t projected_military_spending_copy = projected_military_spending.get_untracked();
	const fixed_point_t projected_social_spending_copy = projected_social_spending.get_untracked();
	const fixed_point_t projected_import_subsidies_copy = projected_import_subsidies.get_untracked();
	if (projected_spending.get_untracked() <= available_funds) {
		actual_administration_spending.set(projected_administration_spending_copy);
		actual_education_spending.set(projected_education_spending_copy);
		actual_military_spending.set(projected_military_spending_copy);
		actual_social_spending.set(projected_social_spending_copy);
		actual_import_subsidies = projected_import_subsidies_copy;
	} else {
		//TODO try take loan (callback?)
		//update available_funds with loan

		if (available_funds < projected_education_spending_copy) {
			actual_education_spending.set(available_funds);
			actual_administration_spending.set(0);
			actual_military_spending.set(0);
			actual_social_spending.set(0);
		} else {
			available_funds -= projected_education_spending_copy;
			actual_education_spending.set(projected_education_spending_copy);

			if (available_funds < projected_administration_spending_copy) {
				actual_administration_spending.set(available_funds);
				actual_military_spending.set(0);
				actual_social_spending.set(0);
			} else {
				available_funds -= projected_administration_spending_copy;
				actual_administration_spending.set(projected_administration_spending_copy);

				if (available_funds < projected_social_spending_copy) {
					actual_social_spending.set(available_funds);
					actual_military_spending.set(0);
				} else {
					available_funds -= projected_social_spending_copy;
					actual_social_spending.set(projected_social_spending_copy);

					if (available_funds < projected_military_spending_copy) {
						actual_military_spending.set(available_funds);
						actual_import_subsidies = 0;
					} else {
						available_funds -= projected_military_spending_copy;
						actual_military_spending.set(projected_military_spending_copy);

						actual_import_subsidies = std::min(available_funds, projected_import_subsidies_copy);
					}
				}
			}
		}
	}

	for (auto& data : goods_data.get_values()) {
		data.clear_daily_recorded_data();
	}

	actual_net_tariffs.set(
		has_import_subsidies.get_untracked()
		? -actual_import_subsidies
		: fixed_point_t::_0
	);
	taxable_income_by_pop_type.fill(0);
	const fixed_point_t total_expenses = actual_import_subsidies
		+ actual_administration_spending.get_untracked()
		+ actual_education_spending.get_untracked()
		+ actual_military_spending.get_untracked()
		+ actual_social_spending.get_untracked();
		//TODO: + factory subsidies
		//TODO: + interest
		//TODO: + diplomatic costs
	cash_stockpile -= total_expenses;
}

void CountryInstance::country_tick_after_map(InstanceManager& instance_manager) {
	DefinitionManager const& definition_manager = instance_manager.get_definition_manager();
	DefineManager const& define_manager = definition_manager.get_define_manager();

	// Gain daily research points
	research_point_stockpile += daily_research_points.get_untracked();

	Technology const* current_research_copy = current_research.get_untracked();
	if (current_research_copy != nullptr) {
		const fixed_point_t research_points_spent = std::min(
			std::min(
				research_point_stockpile.get_untracked(),
				current_research_cost.get_untracked() - invested_research_points.get_untracked()
			),
			define_manager.get_economy_defines().get_max_daily_research()
		);

		research_point_stockpile -= research_points_spent;
		invested_research_points += research_points_spent;

		if (invested_research_points.get_untracked() >= current_research_cost.get_untracked()) {
			unlock_technology(*current_research_copy, instance_manager.get_good_instance_manager());
			current_research.set(nullptr);
			invested_research_points.set(0);
			current_research_cost.set(0);
		}
	}

	// Apply maximum research point stockpile limit
	const fixed_point_t max_research_point_stockpile = is_civilised()
		? daily_research_points.get_untracked() * static_cast<int32_t>(Date::DAYS_IN_YEAR)
		: define_manager.get_country_defines().get_max_research_points();
	if (research_point_stockpile.get_untracked() > max_research_point_stockpile) {
		research_point_stockpile.set(max_research_point_stockpile);
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

	fixed_point_t total_gold_production = 0;
	for (auto const& [good_instance, data] : goods_data) {
		GoodDefinition const& good_definition = good_instance.get_good_definition();
		if (!good_definition.get_is_money()) {
			continue;
		}

		for (auto const& [production_type, produced_quantity] : data.production_per_production_type) {
			if (production_type->get_template_type() != ProductionType::template_type_t::RGO) {
				continue;
			}
			total_gold_production += produced_quantity;
		}
	}

	const fixed_point_t gold_income_value = country_defines.get_gold_to_cash_rate() * total_gold_production;;
	gold_income.set(gold_income_value);
	cash_stockpile += gold_income_value;
}

CountryInstance::good_data_t::good_data_t()
	: mutex { memory::make_unique<std::mutex>() }
	{ }

void CountryInstance::good_data_t::clear_daily_recorded_data() {
	const std::lock_guard<std::mutex> lock_guard { *mutex };
	stockpile_change_yesterday
		= exported_amount
		= government_needs
		= army_needs
		= navy_needs
		= overseas_maintenance
		= factory_demand
		= pop_demand
		= available_amount
		= 0;
	need_consumption_per_pop_type.clear();
	input_consumption_per_production_type.clear();
	production_per_production_type.clear();
}

void CountryInstance::report_pop_income_tax(PopType const& pop_type, const fixed_point_t gross_income, const fixed_point_t paid_as_tax) {
	const std::lock_guard<std::mutex> lock_guard { taxable_income_mutex };
	taxable_income_by_pop_type.at(pop_type) += gross_income;
	cash_stockpile += paid_as_tax;
}

void CountryInstance::report_pop_need_consumption(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.need_consumption_per_pop_type[&pop_type] += quantity;
}
void CountryInstance::report_pop_need_demand(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.pop_demand += quantity;
}
void CountryInstance::report_input_consumption(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.input_consumption_per_production_type[&production_type] += quantity;
}
void CountryInstance::report_input_demand(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity) {
	if (production_type.get_template_type() == ProductionType::template_type_t::ARTISAN) {
		switch (game_rules_manager.get_artisanal_input_demand_category()) {
			case demand_category::FactoryNeeds: break;
			case demand_category::PopNeeds: {
				good_data_t& good_data = get_good_data(good);
				const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
				good_data.pop_demand += quantity;
				return;
			}
			default: return; //demand_category::None
		}
	}

	good_data_t& good_data = get_good_data(good);
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.factory_demand += quantity;
}
void CountryInstance::report_output(ProductionType const& production_type, const fixed_point_t quantity) {
	good_data_t& good_data = get_good_data(production_type.get_output_good());
	const std::lock_guard<std::mutex> lock_guard { *good_data.mutex };
	good_data.production_per_production_type[&production_type] += quantity;
}

void CountryInstance::request_salaries_and_welfare_and_import_subsidies(Pop& pop) {
	PopType const& pop_type = *pop.get_type();
	const pop_size_t pop_size = pop.get_size();
	SharedPopTypeValues const& pop_type_values = shared_country_values.get_shared_pop_type_values(pop_type);
	ModifierEffectCache const& modifier_effect_cache = shared_country_values.get_modifier_effect_cache();

	const fixed_point_t actual_administration_spending_value = actual_administration_spending.get_untracked();
	if (actual_administration_spending_value > fixed_point_t::_0) {
		const fixed_point_t administration_salary = fixed_point_t::mul_div(
			pop_size * administration_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_administration_spending_value,
			projected_administration_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (administration_salary > fixed_point_t::_0) {
			pop.add_government_salary_administration(administration_salary);
		}
	}

	const fixed_point_t actual_education_spending_value = actual_education_spending.get_untracked();
	if (actual_education_spending_value > fixed_point_t::_0) {
		const fixed_point_t education_salary = fixed_point_t::mul_div(
			pop_size * education_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_education_spending_value,
			projected_education_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (education_salary > fixed_point_t::_0) {
			pop.add_government_salary_education(education_salary);
		}
	}

	const fixed_point_t actual_military_spending_value = actual_military_spending.get_untracked();
	if (actual_military_spending_value > fixed_point_t::_0) {
		const fixed_point_t military_salary = fixed_point_t::mul_div(
			pop_size * military_salary_base_by_pop_type.at(pop_type).get_untracked(),
			actual_military_spending_value,
			projected_military_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (military_salary > fixed_point_t::_0) {
			pop.add_government_salary_military(military_salary);
		}
	}

	const fixed_point_t actual_social_spending_value = actual_social_spending.get_untracked();
	if (actual_social_spending_value > fixed_point_t::_0) {
		const fixed_point_t pension_income = fixed_point_t::mul_div(
			pop_size * calculate_pensions_base(modifier_effect_cache, pop_type),
			actual_social_spending_value,
			projected_social_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (pension_income > fixed_point_t::_0) {
			pop.add_pensions(pension_income);
		}

		const fixed_point_t unemployment_subsidies = fixed_point_t::mul_div(
			pop.get_unemployed() * calculate_unemployment_subsidies_base(modifier_effect_cache, pop_type),
			actual_social_spending_value,
			projected_social_spending_unscaled_by_slider.get_untracked()
		) / Pop::size_denominator;
		if (unemployment_subsidies > fixed_point_t::_0) {
			pop.add_unemployment_subsidies(unemployment_subsidies);
		}
	}

	const fixed_point_t actual_net_tariffs_value = actual_net_tariffs.get_untracked();
	if (actual_net_tariffs_value < fixed_point_t::_0) {
		const fixed_point_t import_subsidies = fixed_point_t::mul_div(
			effective_tariff_rate.get_untracked() // < 0
				* pop.get_yesterdays_import_value().get_copy_of_value(),
			actual_net_tariffs_value, // < 0
			projected_import_subsidies.get_untracked() // > 0
		); //effective_tariff_rate * actual_net_tariffs cancel out the negative
		pop.add_import_subsidies(import_subsidies);
	}
}

fixed_point_t CountryInstance::calculate_minimum_wage_base(PopType const& pop_type) {
	if (pop_type.get_is_slave()) {
		return 0;
	}

	return calculate_minimum_wage_base(
		shared_country_values.get_modifier_effect_cache(),
		pop_type
	);
}

fixed_point_t CountryInstance::apply_tariff(const fixed_point_t money_spent_on_imports) {
	const fixed_point_t effective_tariff_rate_value = effective_tariff_rate.get_untracked();
	if (effective_tariff_rate_value <= 0) {
		return 0;
	}

	const fixed_point_t tariff = effective_tariff_rate_value * money_spent_on_imports;
	const std::lock_guard<std::mutex> lock_guard { actual_net_tariffs_mutex };
	actual_net_tariffs += tariff;
	return tariff;
}

CountryInstance::good_data_t& CountryInstance::get_good_data(GoodInstance const& good_instance) {
	return goods_data.at(good_instance);
}
CountryInstance::good_data_t const& CountryInstance::get_good_data(GoodInstance const& good_instance) const {
	return goods_data.at(good_instance);
}
CountryInstance::good_data_t& CountryInstance::get_good_data(GoodDefinition const& good_definition) {
	return goods_data.at_index(good_definition.get_index());
}
CountryInstance::good_data_t const& CountryInstance::get_good_data(GoodDefinition const& good_definition) const {
	return goods_data.at_index(good_definition.get_index());
}

void CountryInstanceManager::update_rankings(Date today, DefineManager const& define_manager) {
	total_ranking.clear();

	for (CountryInstance& country : get_country_instances()) {
		if (country.exists()) {
			total_ranking.push_back(&country);
		}
	}

	prestige_ranking = total_ranking;
	industrial_power_ranking = total_ranking;
	military_power_ranking = total_ranking;

	std::sort(
		total_ranking.begin(), total_ranking.end(),
		[](CountryInstance* a, CountryInstance* b) -> bool {
			const bool a_civilised = a->is_civilised();
			const bool b_civilised = b->is_civilised();
			return a_civilised != b_civilised ? a_civilised : a->total_score.get_untracked() > b->total_score.get_untracked();
		}
	);
	std::sort(
		prestige_ranking.begin(), prestige_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_prestige_untracked() > b->get_prestige_untracked();
		}
	);
	std::sort(
		industrial_power_ranking.begin(), industrial_power_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_industrial_power_untracked() > b->get_industrial_power_untracked();
		}
	);
	std::sort(
		military_power_ranking.begin(), military_power_ranking.end(),
		[](CountryInstance* a, CountryInstance* b) -> bool {
			return a->military_power.get_untracked() > b->military_power.get_untracked();
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
	std::erase_if(great_powers, [max_great_power_rank, today](CountryInstance* great_power) -> bool {
		if (OV_likely(great_power->get_country_status() == COUNTRY_STATUS_GREAT_POWER)) {
			if (OV_unlikely(
				great_power->get_total_rank() > max_great_power_rank && great_power->get_lose_great_power_date() < today
			)) {
				great_power->country_status = COUNTRY_STATUS_CIVILISED;
				return true;
			} else {
				return false;
			}
		}
		return true;
	});

	// Demote all secondary powers and clear the list. We will rebuilt the whole list from scratch, so there's no need to
	// keep countries which are still above the max secondary power rank (they might become great powers instead anyway).
	for (CountryInstance* secondary_power : secondary_powers) {
		if (secondary_power->country_status == COUNTRY_STATUS_SECONDARY_POWER) {
			secondary_power->country_status = COUNTRY_STATUS_CIVILISED;
		}
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

CountryInstanceManager::CountryInstanceManager(
	CountryDefinitionManager const& new_country_definition_manager,
	ModifierEffectCache const& new_modifier_effect_cache,
	CountryDefines const& new_country_defines,
	PopsDefines const& new_pop_defines,			
	decltype(CountryInstance::building_type_unlock_levels)::keys_span_type building_type_keys,
	decltype(CountryInstance::technology_unlock_levels)::keys_span_type technology_keys,
	decltype(CountryInstance::invention_unlock_levels)::keys_span_type invention_keys,
	decltype(CountryInstance::reforms)::keys_span_type reform_keys,
	decltype(CountryInstance::flag_overrides_by_government_type)::keys_span_type government_type_keys,
	decltype(CountryInstance::crime_unlock_levels)::keys_span_type crime_keys,
	decltype(CountryInstance::goods_data)::keys_span_type good_instances_keys,
	decltype(CountryInstance::regiment_type_unlock_levels)::keys_span_type regiment_type_unlock_levels_keys,
	decltype(CountryInstance::ship_type_unlock_levels)::keys_span_type ship_type_unlock_levels_keys,
	utility::forwardable_span<const Strata> strata_keys,
	utility::forwardable_span<const PopType> pop_type_keys,
	utility::forwardable_span<const Ideology> ideology_keys,
	GameRulesManager const& game_rules_manager,
	CountryRelationManager& country_relations_manager,
	GoodInstanceManager& good_instance_manager,
	EconomyDefines const& economy_defines
)
  : country_definition_manager { new_country_definition_manager },
	shared_country_values {
		new_modifier_effect_cache,
		new_country_defines,
		new_pop_defines,
		pop_type_keys
	},
	country_instance_by_definition {
		new_country_definition_manager.get_country_definitions(),
		[
			this,
			building_type_keys,
			technology_keys,
			invention_keys,
			reform_keys,
			government_type_keys,
			crime_keys,
			good_instances_keys,
			regiment_type_unlock_levels_keys,
			ship_type_unlock_levels_keys,
			strata_keys,
			pop_type_keys,
			ideology_keys,
			game_rules_manager_ptr=&game_rules_manager,
			country_relations_manager_ptr=&country_relations_manager,
			good_instance_manager_ptr=&good_instance_manager,
			new_country_defines_ptr=&new_country_defines,
			economy_defines_ptr=&economy_defines
		](CountryDefinition const& country_definition)->auto{
			return std::make_tuple(
				&country_definition,
				building_type_keys,
				technology_keys,
				invention_keys,
				reform_keys,
				government_type_keys,
				crime_keys,
				good_instances_keys,
				regiment_type_unlock_levels_keys,
				ship_type_unlock_levels_keys,
				strata_keys,
				pop_type_keys,
				ideology_keys,
				game_rules_manager_ptr,
				country_relations_manager_ptr,
				&shared_country_values,
				good_instance_manager_ptr,
				new_country_defines_ptr,
				economy_defines_ptr
			);
		}
	}
{
	assert(new_country_definition_manager.country_definitions_are_locked());
	great_powers.reserve(16);
	secondary_powers.reserve(16);
}

CountryInstance* CountryInstanceManager::get_country_instance_by_identifier(std::string_view identifier) {
	CountryDefinition const* country_definition = country_definition_manager.get_country_definition_by_identifier(identifier);
	return country_definition == nullptr
		? nullptr
		: &get_country_instance_by_definition(*country_definition);
}
CountryInstance const* CountryInstanceManager::get_country_instance_by_identifier(std::string_view identifier) const {
	CountryDefinition const* country_definition = country_definition_manager.get_country_definition_by_identifier(identifier);
	return country_definition == nullptr
		? nullptr
		: &get_country_instance_by_definition(*country_definition);
}
CountryInstance* CountryInstanceManager::get_country_instance_by_index(typename CountryInstance::index_t index) {
	return country_instance_by_definition.contains_index(index)
		? &country_instance_by_definition.at_index(index)
		: nullptr;
}
CountryInstance const* CountryInstanceManager::get_country_instance_by_index(typename CountryInstance::index_t index) const {
	return country_instance_by_definition.contains_index(index)
		? &country_instance_by_definition.at_index(index)
		: nullptr;
}
CountryInstance& CountryInstanceManager::get_country_instance_by_definition(CountryDefinition const& country_definition) {
	return country_instance_by_definition.at(country_definition);
}
CountryInstance const& CountryInstanceManager::get_country_instance_by_definition(CountryDefinition const& country_definition) const {
	return country_instance_by_definition.at(country_definition);
}

bool CountryInstanceManager::apply_history_to_countries(InstanceManager& instance_manager) {
	CountryHistoryManager const& history_manager =
		instance_manager.get_definition_manager().get_history_manager().get_country_manager();

	bool ret = true;

	const Date today = instance_manager.get_today();
	UnitInstanceManager& unit_instance_manager = instance_manager.get_unit_instance_manager();
	MapInstance& map_instance = instance_manager.get_map_instance();

	const Date starting_last_war_loss_date = today - RECENT_WAR_LOSS_TIME_LIMIT;

	for (CountryInstance& country_instance : get_country_instances()) {
		country_instance.last_war_loss_date = starting_last_war_loss_date;

		if (!country_instance.get_country_definition().is_dynamic_tag()) {
			CountryHistoryMap const* history_map = history_manager.get_country_history(
				country_instance.get_country_definition()
			);

			if (history_map != nullptr) {
				static constexpr fixed_point_t DEFAULT_STATE_CULTURE_LITERACY = fixed_point_t::_0_50;
				CountryHistoryEntry const* oob_history_entry = nullptr;
				std::optional<fixed_point_t> state_culture_consciousness;
				std::optional<fixed_point_t> nonstate_culture_consciousness;
				fixed_point_t state_culture_literacy = DEFAULT_STATE_CULTURE_LITERACY;
				std::optional<fixed_point_t> nonstate_culture_literacy;

				for (auto const& [entry_date, entry] : history_map->get_entries()) {
					if (entry_date <= today) {
						ret &= country_instance.apply_history_to_country(*entry, instance_manager);

						if (entry->get_initial_oob().has_value()) {
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
						map_instance, country_instance, *oob_history_entry->get_initial_oob()
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
	shared_country_values.update_costs(instance_manager.get_good_instance_manager());
	return ret;
}

void CountryInstanceManager::update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache) {
	for (CountryInstance& country : get_country_instances()) {
		country.update_modifier_sum(today, static_modifier_cache);
	}
}

void CountryInstanceManager::update_gamestate(InstanceManager& instance_manager) {
	for (CountryInstance& country : get_country_instances()) {
		country.update_gamestate(instance_manager);
	}

	// TODO - work out how to have ranking effects applied (e.g. static modifiers) applied at game start
	// we can't just move update_rankings to the top of this function as it will choose initial GPs based on
	// incomplete scores. Although we should check if the base game includes all info or if it really does choose
	// starting GPs based purely on stuff like prestige which is set by history before the first game update.
	update_rankings(instance_manager.get_today(), instance_manager.get_definition_manager().get_define_manager());
}

void CountryInstanceManager::country_manager_tick_before_map(InstanceManager& instance_manager) {
	//TODO parallellise?
	for (CountryInstance& country : get_country_instances()) {
		country.country_tick_before_map(instance_manager);
	}
}

void CountryInstanceManager::country_manager_tick_after_map(InstanceManager& instance_manager) {
	//TODO parallellise?
	for (CountryInstance& country : get_country_instances()) {
		country.country_tick_after_map(instance_manager);
	}

	shared_country_values.update_costs(instance_manager.get_good_instance_manager());
}
