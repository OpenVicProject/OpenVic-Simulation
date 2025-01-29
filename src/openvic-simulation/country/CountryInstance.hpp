#pragma once

#include <utility>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Rule.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/SliderValue.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryInstanceManager;
	struct CountryDefinition;
	struct ProvinceInstance;
	struct State;
	struct Technology;
	struct Invention;
	struct TechnologySchool;
	struct NationalValue;
	struct GovernmentType;
	struct CountryParty;
	struct Ideology;
	struct ReformGroup;
	struct Reform;
	struct Crime;
	struct Culture;
	struct Religion;
	struct BuildingType;
	struct CountryHistoryEntry;
	struct DefineManager;
	struct ModifierEffectCache;
	struct StaticModifierCache;
	struct InstanceManager;

	/* Representation of a country's mutable attributes, with a CountryDefinition that is unique at any single time
	 * but can be swapped with other CountryInstance's CountryDefinition when switching tags. */
	struct CountryInstance : FlagStrings {
		friend struct CountryInstanceManager;

		/*
			Westernisation Progress vs Status for Uncivilised Countries:
				15 - primitive
				16 - uncivilised
				50 - uncivilised
				51 - partially westernised
		*/

		enum struct country_status_t : uint8_t {
			COUNTRY_STATUS_GREAT_POWER,
			COUNTRY_STATUS_SECONDARY_POWER,
			COUNTRY_STATUS_CIVILISED,
			COUNTRY_STATUS_PARTIALLY_CIVILISED,
			COUNTRY_STATUS_UNCIVILISED,
			COUNTRY_STATUS_PRIMITIVE
		};

		// Thresholds for different uncivilised country statuses
		static constexpr fixed_point_t PRIMITIVE_CIVILISATION_PROGRESS = fixed_point_t::parse(15) / 100;
		static constexpr fixed_point_t UNCIVILISED_CIVILISATION_PROGRESS = fixed_point_t::_0_50();

		using unlock_level_t = int8_t;
		using unit_variant_t = uint8_t;

		static constexpr slider_value_int_type SLIDER_SIZE = 100;

		using StandardSliderValue = SliderValue<0, SLIDER_SIZE>;
		using TariffSliderValue = SliderValue<-SLIDER_SIZE, SLIDER_SIZE>;

	private:
		/* Main attributes */
		// We can always assume country_definition is not null, as it is initialised from a reference and only ever changed
		// by swapping with another CountryInstance's country_definition.
		CountryDefinition const* PROPERTY(country_definition);
		colour_t PROPERTY(colour); // Cached to avoid searching government overrides for every province
		ProvinceInstance* PROPERTY_PTR(capital, nullptr);
		bool PROPERTY_CUSTOM_PREFIX(releasable_vassal, is, true);
		bool PROPERTY(owns_colonial_province, false);

		country_status_t PROPERTY(country_status, country_status_t::COUNTRY_STATUS_UNCIVILISED);
		fixed_point_t PROPERTY(civilisation_progress);
		Date PROPERTY(lose_great_power_date);
		fixed_point_t PROPERTY(total_score);
		size_t PROPERTY(total_rank, 0);

		ordered_set<ProvinceInstance*> PROPERTY(owned_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(controlled_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(core_provinces);
		ordered_set<State*> PROPERTY(states);

		ordered_set<CountryInstance*> PROPERTY(neighbouring_countries);

		string_map_t<fixed_point_t> PROPERTY(script_variables);

		// The total/resultant modifier affecting this country, including owned province contributions.
		ModifierSum PROPERTY(modifier_sum);
		std::vector<ModifierInstance> PROPERTY(event_modifiers);

		/* Production */
		fixed_point_t PROPERTY(industrial_power);
		std::vector<std::pair<State const*, fixed_point_t>> PROPERTY(industrial_power_from_states);
		std::vector<std::pair<CountryInstance const*, fixed_point_t>> PROPERTY(industrial_power_from_investments);
		size_t PROPERTY(industrial_rank, 0);
		fixed_point_map_t<CountryInstance const*> PROPERTY(foreign_investments);
		IndexedMap<BuildingType, unlock_level_t> PROPERTY(building_type_unlock_levels);
		// TODO - total amount of each good produced

		/* Budget */
		fixed_point_t PROPERTY(cash_stockpile);
		IndexedMap<Strata, StandardSliderValue> PROPERTY(tax_rate_by_strata);
		StandardSliderValue PROPERTY(land_spending);
		StandardSliderValue PROPERTY(naval_spending);
		StandardSliderValue PROPERTY(construction_spending);
		StandardSliderValue PROPERTY(education_spending);
		StandardSliderValue PROPERTY(administration_spending);
		StandardSliderValue PROPERTY(social_spending);
		StandardSliderValue PROPERTY(military_spending);
		TariffSliderValue PROPERTY(tariff_rate);
		// TODO - cash stockpile change over last 30 days

		/* Technology */
		IndexedMap<Technology, unlock_level_t> PROPERTY(technology_unlock_levels);
		IndexedMap<Invention, unlock_level_t> PROPERTY(invention_unlock_levels);
		Technology const* PROPERTY(current_research, nullptr);
		fixed_point_t PROPERTY(invested_research_points);
		fixed_point_t PROPERTY(current_research_cost);
		Date PROPERTY(expected_research_completion_date);
		fixed_point_t PROPERTY(research_point_stockpile);
		fixed_point_t PROPERTY(daily_research_points);
		fixed_point_map_t<PopType const*> PROPERTY(research_points_from_pop_types);
		fixed_point_t PROPERTY(national_literacy);
		TechnologySchool const* PROPERTY(tech_school, nullptr);
		// TODO - cached possible inventions with %age chance

		/* Politics */
		NationalValue const* PROPERTY(national_value, nullptr);
		GovernmentType const* PROPERTY(government_type, nullptr);
		Date PROPERTY(last_election);
		CountryParty const* PROPERTY(ruling_party, nullptr);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(upper_house);
		IndexedMap<ReformGroup, Reform const*> PROPERTY(reforms);
		fixed_point_t PROPERTY(total_administrative_multiplier);
		RuleSet PROPERTY(rule_set);
		// TODO - national issue support distribution (for just voters and for everyone)
		IndexedMap<GovernmentType, GovernmentType const*> PROPERTY(government_flag_overrides);
		GovernmentType const* PROPERTY(flag_government_type, nullptr);
		fixed_point_t PROPERTY(suppression_points);
		fixed_point_t PROPERTY(infamy); // in 0-25+ range
		fixed_point_t PROPERTY(plurality); // in 0-100 range
		fixed_point_t PROPERTY(revanchism);
		IndexedMap<Crime, unlock_level_t> PROPERTY(crime_unlock_levels);
		// TODO - rebel movements

		/* Population */
		Culture const* PROPERTY(primary_culture, nullptr);
		ordered_set<Culture const*> PROPERTY(accepted_cultures);
		Religion const* PROPERTY(religion, nullptr);
		pop_size_t PROPERTY(total_population, 0);
		// TODO - population change over last 30 days
		fixed_point_t PROPERTY(national_consciousness);
		fixed_point_t PROPERTY(national_militancy);

		IndexedMap<Strata, pop_size_t> PROPERTY(population_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(militancy_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(life_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(everyday_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(luxury_needs_fulfilled_by_strata);

		IndexedMap<PopType, pop_size_t> PROPERTY(pop_type_distribution);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Issue const*> PROPERTY(issue_distribution);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(vote_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);
		size_t PROPERTY(national_focus_capacity, 0);
		// TODO - national foci

		/* Trade */
		// TODO - total amount of each good exported and imported

		/* Diplomacy */
		fixed_point_t PROPERTY(prestige);
		size_t PROPERTY(prestige_rank, 0);
		fixed_point_t PROPERTY(diplomatic_points);
		// TODO - colonial power, current wars

		/* Military */
		fixed_point_t PROPERTY(military_power);
		fixed_point_t PROPERTY(military_power_from_land);
		fixed_point_t PROPERTY(military_power_from_sea);
		fixed_point_t PROPERTY(military_power_from_leaders);
		size_t PROPERTY(military_rank, 0);
		plf::colony<General> PROPERTY(generals);
		plf::colony<Admiral> PROPERTY(admirals);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);
		size_t PROPERTY(regiment_count, 0);
		size_t PROPERTY(max_supported_regiment_count, 0);
		size_t PROPERTY(mobilisation_potential_regiment_count, 0);
		size_t PROPERTY(mobilisation_max_regiment_count, 0);
		fixed_point_t PROPERTY(mobilisation_impact);
		fixed_point_t PROPERTY(mobilisation_economy_impact);
		fixed_point_t PROPERTY(supply_consumption);
		size_t PROPERTY(ship_count, 0);
		fixed_point_t PROPERTY(total_consumed_ship_supply);
		fixed_point_t PROPERTY(max_ship_supply);
		fixed_point_t PROPERTY(leadership_point_stockpile);
		fixed_point_t PROPERTY(monthly_leadership_points);
		fixed_point_map_t<PopType const*> PROPERTY(leadership_points_from_pop_types);
		int32_t PROPERTY(create_leader_count, 0);
		// War exhaustion is stored as a raw decimal rather than a proportion, it is usually in the range 0-100.
		// The current war exhaustion value should only ever be modified via change_war_exhaustion(delta) which also
		// clamps the result in the range [0, war_exhaustion_max] straight away, matching the base game's behaviour.
		fixed_point_t PROPERTY(war_exhaustion);
		fixed_point_t PROPERTY(war_exhaustion_max);
		bool PROPERTY_CUSTOM_PREFIX(mobilised, is, false);
		bool PROPERTY_CUSTOM_PREFIX(disarmed, is, false);
		bool PROPERTY_RW(auto_create_leaders, true);
		bool PROPERTY_RW(auto_assign_leaders, true);
		fixed_point_t PROPERTY(organisation_regain);
		fixed_point_t PROPERTY(land_organisation);
		fixed_point_t PROPERTY(naval_organisation);
		fixed_point_t PROPERTY(land_unit_start_experience);
		fixed_point_t PROPERTY(naval_unit_start_experience);
		fixed_point_t PROPERTY(recruit_time);
		int32_t PROPERTY(combat_width, 1);
		int32_t PROPERTY(digin_cap, 0);
		fixed_point_t PROPERTY(military_tactics);
		IndexedMap<RegimentType, unlock_level_t> PROPERTY(regiment_type_unlock_levels);
		RegimentType::allowed_cultures_t PROPERTY(allowed_regiment_cultures, RegimentType::allowed_cultures_t::NO_CULTURES);
		IndexedMap<ShipType, unlock_level_t> PROPERTY(ship_type_unlock_levels);
		unlock_level_t PROPERTY(gas_attack_unlock_level, 0);
		unlock_level_t PROPERTY(gas_defence_unlock_level, 0);
		std::vector<unlock_level_t> PROPERTY(unit_variant_unlock_levels);

		CountryInstance(
			CountryDefinition const* new_country_definition,
			decltype(building_type_unlock_levels)::keys_type const& building_type_keys,
			decltype(technology_unlock_levels)::keys_type const& technology_keys,
			decltype(invention_unlock_levels)::keys_type const& invention_keys,
			decltype(upper_house)::keys_type const& ideology_keys,
			decltype(reforms)::keys_type const& reform_keys,
			decltype(government_flag_overrides)::keys_type const& government_type_keys,
			decltype(crime_unlock_levels)::keys_type const& crime_keys,
			decltype(pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(regiment_type_unlock_levels)::keys_type const& regiment_type_unlock_levels_keys,
			decltype(ship_type_unlock_levels)::keys_type const& ship_type_unlock_levels_keys,
			decltype(tax_rate_by_strata)::keys_type const& strata_keys
		);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		UNIT_BRANCHED_GETTER(get_unit_type_unlock_levels, regiment_type_unlock_levels, ship_type_unlock_levels);

	public:
		UNIT_BRANCHED_GETTER(get_leaders, generals, admirals);
		UNIT_BRANCHED_GETTER_CONST(get_leaders, generals, admirals);

		inline size_t get_general_count() const {
			return generals.size();
		}
		inline size_t has_generals() const {
			return !generals.empty();
		}
		inline size_t get_admiral_count() const {
			return admirals.size();
		}
		inline size_t has_admirals() const {
			return !admirals.empty();
		}
		inline size_t get_leader_count() const {
			return get_general_count() + get_admiral_count();
		}
		inline size_t has_leaders() const {
			return has_generals() || has_admirals();
		}
		inline size_t get_army_count() const {
			return armies.size();
		}
		inline size_t has_armies() const {
			return !armies.empty();
		}
		inline size_t get_navy_count() const {
			return navies.size();
		}
		inline size_t has_navies() const {
			return !navies.empty();
		}

		std::string_view get_identifier() const;

		void update_country_definition_based_attributes();

		bool exists() const;
		bool is_civilised() const;
		bool can_colonise() const;
		bool is_great_power() const;
		bool is_secondary_power() const;
		bool is_at_war() const;
		bool is_neighbour(CountryInstance const& country) const;

		// These functions take "std::string const&" rather than "std::string_view" as they're only used with script arguments
		// which are always stored as "std::string"s and it significantly simplifies mutable value access.
		fixed_point_t get_script_variable(std::string const& variable_name) const;
		void set_script_variable(std::string const& variable_name, fixed_point_t value);
		// Adds the argument value to the existing value of the script variable (initialised to 0 if it doesn't already exist).
		void change_script_variable(std::string const& variable_name, fixed_point_t value);

		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0
		constexpr fixed_point_t get_pop_type_proportion(PopType const& pop_type) const {
			return pop_type_distribution[pop_type];
		}
		constexpr fixed_point_t get_ideology_support(Ideology const& ideology) const {
			return ideology_distribution[ideology];
		}
		fixed_point_t get_issue_support(Issue const& issue) const;
		fixed_point_t get_party_support(CountryParty const& party) const;
		fixed_point_t get_culture_proportion(Culture const& culture) const;
		fixed_point_t get_religion_proportion(Religion const& religion) const;
		constexpr pop_size_t get_strata_population(Strata const& strata) const {
			return population_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_militancy(Strata const& strata) const {
			return militancy_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_life_needs_fulfilled(Strata const& strata) const {
			return life_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_everyday_needs_fulfilled(Strata const& strata) const {
			return everyday_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_luxury_needs_fulfilled(Strata const& strata) const {
			return luxury_needs_fulfilled_by_strata[strata];
		}

		bool add_owned_province(ProvinceInstance& new_province);
		bool remove_owned_province(ProvinceInstance const& province_to_remove);
		bool has_owned_province(ProvinceInstance const& province) const;

		bool add_controlled_province(ProvinceInstance& new_province);
		bool remove_controlled_province(ProvinceInstance const& province_to_remove);
		bool has_controlled_province(ProvinceInstance const& province) const;

		bool add_core_province(ProvinceInstance& new_core);
		bool remove_core_province(ProvinceInstance const& core_to_remove);
		bool has_core_province(ProvinceInstance const& province) const;

		bool add_state(State& new_state);
		bool remove_state(State const& state_to_remove);
		bool has_state(State const& state) const;

		bool add_accepted_culture(Culture const& new_accepted_culture);
		bool remove_accepted_culture(Culture const& culture_to_remove);
		bool has_accepted_culture(Culture const& culture) const;

		/* Set a party's popularity in the upper house. */
		bool set_upper_house(Ideology const* ideology, fixed_point_t popularity);
		bool set_ruling_party(CountryParty const& new_ruling_party);
		bool add_reform(Reform const& new_reform);

		void set_strata_tax_rate(Strata const& strata, const StandardSliderValue::int_type new_value);
		void set_land_spending(const StandardSliderValue::int_type new_value);
		void set_naval_spending(const StandardSliderValue::int_type new_value);
		void set_construction_spending(const StandardSliderValue::int_type new_value);
		void set_education_spending(const StandardSliderValue::int_type new_value);
		void set_administration_spending(const StandardSliderValue::int_type new_value);
		void set_social_spending(const StandardSliderValue::int_type new_value);
		void set_military_spending(const StandardSliderValue::int_type new_value);
		void set_tariff_rate(const StandardSliderValue::int_type new_value);

		// Adds delta to the current war exhaustion value and clamps it to the range [0, war_exhaustion_max].
		void change_war_exhaustion(fixed_point_t delta);

		template<UnitType::branch_t Branch>
		bool add_unit_instance_group(UnitInstanceGroup<Branch>& group);
		template<UnitType::branch_t Branch>
		bool remove_unit_instance_group(UnitInstanceGroup<Branch> const& group);

		template<UnitType::branch_t Branch>
		void add_leader(LeaderBranched<Branch>&& leader);
		template<UnitType::branch_t Branch>
		bool remove_leader(LeaderBranched<Branch> const& leader);

		bool has_leader_with_name(std::string_view name) const;

		template<UnitType::branch_t Branch>
		bool modify_unit_type_unlock(UnitTypeBranched<Branch> const& unit_type, unlock_level_t unlock_level_change);

		bool modify_unit_type_unlock(UnitType const& unit_type, unlock_level_t unlock_level_change);
		bool unlock_unit_type(UnitType const& unit_type);
		bool is_unit_type_unlocked(UnitType const& unit_type) const;

		bool modify_building_type_unlock(BuildingType const& building_type, unlock_level_t unlock_level_change);
		bool unlock_building_type(BuildingType const& building_type);
		bool is_building_type_unlocked(BuildingType const& building_type) const;

		bool modify_crime_unlock(Crime const& crime, unlock_level_t unlock_level_change);
		bool unlock_crime(Crime const& crime);
		bool is_crime_unlocked(Crime const& crime) const;

		bool modify_gas_attack_unlock(unlock_level_t unlock_level_change);
		bool unlock_gas_attack();
		bool is_gas_attack_unlocked() const;

		bool modify_gas_defence_unlock(unlock_level_t unlock_level_change);
		bool unlock_gas_defence();
		bool is_gas_defence_unlocked() const;

		bool modify_unit_variant_unlock(unit_variant_t unit_variant, unlock_level_t unlock_level_change);
		bool unlock_unit_variant(unit_variant_t unit_variant);
		unit_variant_t get_max_unlocked_unit_variant() const;

		bool modify_technology_unlock(Technology const& technology, unlock_level_t unlock_level_change);
		bool set_technology_unlock_level(Technology const& technology, unlock_level_t unlock_level);
		bool unlock_technology(Technology const& technology);
		bool is_technology_unlocked(Technology const& technology) const;

		bool modify_invention_unlock(Invention const& invention, unlock_level_t unlock_level_change);
		bool set_invention_unlock_level(Invention const& invention, unlock_level_t unlock_level);
		bool unlock_invention(Invention const& invention);
		bool is_invention_unlocked(Invention const& invention) const;

		bool is_primary_culture(Culture const& culture) const;
		// This only checks the accepted cultures list, ignoring the primary culture.
		bool is_accepted_culture(Culture const& culture) const;
		bool is_primary_or_accepted_culture(Culture const& culture) const;

		fixed_point_t calculate_research_cost(
			Technology const& technology, ModifierEffectCache const& modifier_effect_cache
		) const;
		fixed_point_t get_research_progress() const;
		bool can_research_tech(Technology const& technology, Date today) const;
		void start_research(Technology const& technology, InstanceManager const& instance_manager);

		// Sets the investment of each country in the map (rather than adding to them), leaving the rest unchanged.
		void apply_foreign_investments(
			fixed_point_map_t<CountryDefinition const*> const& investments,
			CountryInstanceManager const& country_instance_manager
		);

		bool apply_history_to_country(CountryHistoryEntry const& entry, InstanceManager& instance_manager);

	private:
		void _update_production(DefineManager const& define_manager);
		void _update_budget();
		// Expects current_research to be non-null
		void _update_current_tech(InstanceManager const& instance_manager);
		void _update_technology(InstanceManager const& instance_manager);
		void _update_politics();
		void _update_population();
		void _update_trade();
		void _update_diplomacy();
		void _update_military(
			DefineManager const& define_manager, UnitTypeManager const& unit_type_manager,
			ModifierEffectCache const& modifier_effect_cache
		);

		bool update_rule_set();

	public:

		void update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache);
		void contribute_province_modifier_sum(ModifierSum const& province_modifier_sum);
		fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const;
		constexpr void for_each_contributing_modifier(
			ModifierEffect const& effect, ContributingModifierCallback auto callback
		) const {
			return modifier_sum.for_each_contributing_modifier(effect, std::move(callback));
		}

		void update_gamestate(InstanceManager& instance_manager);
		void tick(InstanceManager& instance_manager);
	};

	struct CountryDefinitionManager;
	struct CountryHistoryManager;

	struct CountryInstanceManager {
	private:
		CountryDefinitionManager const& PROPERTY(country_definition_manager);

		IdentifierRegistry<CountryInstance> IDENTIFIER_REGISTRY(country_instance);

		std::vector<CountryInstance*> PROPERTY(great_powers);
		std::vector<CountryInstance*> PROPERTY(secondary_powers);

		std::vector<CountryInstance*> PROPERTY(total_ranking);
		std::vector<CountryInstance*> PROPERTY(prestige_ranking);
		std::vector<CountryInstance*> PROPERTY(industrial_power_ranking);
		std::vector<CountryInstance*> PROPERTY(military_power_ranking);

		void update_rankings(Date today, DefineManager const& define_manager);

	public:
		CountryInstanceManager(CountryDefinitionManager const& new_country_definition_manager);

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(country_instance);

		CountryInstance& get_country_instance_from_definition(CountryDefinition const& country);
		CountryInstance const& get_country_instance_from_definition(CountryDefinition const& country) const;

		bool generate_country_instances(
			decltype(CountryInstance::building_type_unlock_levels)::keys_type const& building_type_keys,
			decltype(CountryInstance::technology_unlock_levels)::keys_type const& technology_keys,
			decltype(CountryInstance::invention_unlock_levels)::keys_type const& invention_keys,
			decltype(CountryInstance::upper_house)::keys_type const& ideology_keys,
			decltype(CountryInstance::reforms)::keys_type const& reform_keys,
			decltype(CountryInstance::government_flag_overrides)::keys_type const& government_type_keys,
			decltype(CountryInstance::crime_unlock_levels)::keys_type const& crime_keys,
			decltype(CountryInstance::pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(CountryInstance::regiment_type_unlock_levels)::keys_type const& regiment_type_unlock_levels_keys,
			decltype(CountryInstance::ship_type_unlock_levels)::keys_type const& ship_type_unlock_levels_keys,
			decltype(CountryInstance::tax_rate_by_strata):: keys_type const& strata_keys
		);

		bool apply_history_to_countries(CountryHistoryManager const& history_manager, InstanceManager& instance_manager);

		void update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(InstanceManager& instance_manager);
		void tick(InstanceManager& instance_manager);
	};
}
