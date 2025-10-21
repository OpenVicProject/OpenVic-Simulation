#pragma once

#include <utility>

#include <fmt/base.h>

#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/politics/Rule.hpp"
#include "openvic-simulation/pop/PopsAggregate.hpp"
#include "openvic-simulation/types/ClampedValue.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/Atomic.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TechnologyUnlockLevel.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/types/UnitVariant.hpp"
#include "openvic-simulation/types/ValueHistory.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/reactive/DerivedState.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

#include "openvic-simulation/types/IndexedFlatMapMacro.hpp" //below other imports that undef the macros

namespace OpenVic {
	struct BaseIssue;
	struct BuildingType;
	struct CountryDefinition;
	struct CountryDefines;
	struct CountryHistoryEntry;
	struct CountryInstanceManager;
	struct CountryParty;
	struct CountryRelationManager;
	struct Crime;
	struct Culture;
	struct DefineManager;
	struct EconomyDefines;
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodInstance;
	struct GoodInstanceManager;
	struct GovernmentType;
	struct Ideology;
	struct InstanceManager;
	struct Invention;
	struct LeaderInstance;
	struct ModifierEffectCache;
	struct NationalValue;
	struct Pop;
	struct PopType;
	struct PopsDefines;
	struct ProductionType;
	struct ProvinceInstance;
	struct Reform;
	struct ReformGroup;
	struct Religion;
	struct SharedCountryValues;
	struct SharedPopTypeValues;
	struct State;
	struct StaticModifierCache;
	struct Strata;
	struct Technology;
	struct TechnologySchool;
	struct UnitInstanceGroup;
	struct UnitTypeManager;

	static constexpr Timespan RECENT_WAR_LOSS_TIME_LIMIT = Timespan::from_years(5);

	/* Representation of a country's mutable attributes, with a CountryDefinition that is unique at any single time
	 * but can be swapped with other CountryInstance's CountryDefinition when switching tags. */
	struct CountryInstance : FlagStrings, HasIndex<CountryInstance>, PopsAggregate {
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
		static constexpr fixed_point_t UNCIVILISED_CIVILISATION_PROGRESS = fixed_point_t::_0_50;

	private:
		CountryDefinition const& PROPERTY(country_definition);

		GameRulesManager const& game_rules_manager;
		CountryRelationManager& country_relations_manager;
		CountryDefines const& country_defines;
		SharedCountryValues& shared_country_values;

		colour_t PROPERTY(colour); // Cached to avoid searching government overrides for every province
		ProvinceInstance* PROPERTY_PTR(capital, nullptr);
		bool PROPERTY_RW_CUSTOM_NAME(ai, is_ai, set_ai, true);
		bool PROPERTY_CUSTOM_PREFIX(releasable_vassal, is, true);
		bool PROPERTY(owns_colonial_province, false);
		bool PROPERTY(has_unowned_cores, false);
		fixed_point_t PROPERTY(owned_cores_controlled_proportion);
		fixed_point_t PROPERTY(occupied_provinces_proportion);
		size_t PROPERTY(port_count, 0);

		country_status_t PROPERTY(country_status, country_status_t::COUNTRY_STATUS_UNCIVILISED);
		fixed_point_t PROPERTY(civilisation_progress);
		Date PROPERTY(lose_great_power_date);
		size_t PROPERTY(total_rank, 0);

		ordered_set<ProvinceInstance*> PROPERTY(owned_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(controlled_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(core_provinces);
		ordered_set<State*> PROPERTY(states);

		ordered_set<CountryInstance*> PROPERTY(neighbouring_countries);

		string_map_t<fixed_point_t> PROPERTY(script_variables);

		// The total/resultant modifier affecting this country, including owned province contributions.
		ModifierSum PROPERTY(modifier_sum);
		memory::vector<ModifierInstance> PROPERTY(event_modifiers);

		/* Production */
		STATE_PROPERTY(fixed_point_t, industrial_power);
		memory::vector<std::pair<State const*, fixed_point_t>> PROPERTY(industrial_power_from_states);
		memory::vector<std::pair<CountryInstance const*, fixed_point_t>> PROPERTY(industrial_power_from_investments);
		size_t PROPERTY(industrial_rank, 0);
		fixed_point_map_t<CountryInstance const*> PROPERTY(foreign_investments);
		IndexedFlatMap_PROPERTY(BuildingType, technology_unlock_level_t, building_type_unlock_levels);
		// TODO - total amount of each good produced

		/* Budget */
		fixed_point_t cache_stockpile_start_of_tick;
		ValueHistory<fixed_point_t> PROPERTY(balance_history);
		STATE_PROPERTY(fixed_point_t, gold_income);
		atomic_fixed_point_t PROPERTY(cash_stockpile);
		std::mutex taxable_income_mutex;
		IndexedFlatMap_PROPERTY(PopType, fixed_point_t, taxable_income_by_pop_type);
		STATE_PROPERTY(fixed_point_t, tax_efficiency);
		IndexedFlatMap<Strata, DerivedState<fixed_point_t>> PROPERTY(effective_tax_rate_by_strata);
	public:
		DerivedState<fixed_point_t>& get_effective_tax_rate_by_strata(Strata const& strata);
	private:
		IndexedFlatMap<Strata, ClampedValue> tax_rate_slider_value_by_strata;
	public:
		[[nodiscard]] constexpr IndexedFlatMap<Strata, ClampedValue> const& get_tax_rate_slider_value_by_strata() const {
			return tax_rate_slider_value_by_strata;
		}
		[[nodiscard]] ReadOnlyClampedValue& get_tax_rate_slider_value_by_strata(Strata const& strata);
		[[nodiscard]] ReadOnlyClampedValue const& get_tax_rate_slider_value_by_strata(Strata const& strata) const;
	private:

		STATE_PROPERTY(fixed_point_t, administrative_efficiency_from_administrators);
		STATE_PROPERTY(fixed_point_t, administrator_percentage);

		//store per slider per good: desired, bought & cost
		//store purchase record from last tick and prediction next tick
		CLAMPED_PROPERTY(army_spending_slider_value);
		CLAMPED_PROPERTY(navy_spending_slider_value);
		CLAMPED_PROPERTY(construction_spending_slider_value);

		CLAMPED_PROPERTY(administration_spending_slider_value);
		STATE_PROPERTY(fixed_point_t, projected_administration_spending_unscaled_by_slider);
		STATE_PROPERTY(fixed_point_t, actual_administration_spending);

		CLAMPED_PROPERTY(education_spending_slider_value);
		STATE_PROPERTY(fixed_point_t, projected_education_spending_unscaled_by_slider);
		STATE_PROPERTY(fixed_point_t, actual_education_spending);

		CLAMPED_PROPERTY(military_spending_slider_value);
		STATE_PROPERTY(fixed_point_t, projected_military_spending_unscaled_by_slider);
		STATE_PROPERTY(fixed_point_t, actual_military_spending);

		CLAMPED_PROPERTY(social_spending_slider_value);
		STATE_PROPERTY(fixed_point_t, projected_pensions_spending_unscaled_by_slider);
		STATE_PROPERTY(fixed_point_t, projected_unemployment_subsidies_spending_unscaled_by_slider);
		STATE_PROPERTY(fixed_point_t, actual_social_spending);
		
		//base here means not scaled by slider or pop size
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> administration_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> education_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> military_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> social_income_variant_base_by_pop_type;

		CLAMPED_PROPERTY(tariff_rate_slider_value);
		std::mutex actual_net_tariffs_mutex;
		MutableState<fixed_point_t> actual_net_tariffs;

		//TODO actual factory subsidies
		//projected cost is UI only and lists the different factories

		/* Technology */
		IndexedFlatMap_PROPERTY(Technology, technology_unlock_level_t, technology_unlock_levels);
		IndexedFlatMap_PROPERTY(Invention, technology_unlock_level_t, invention_unlock_levels);
		STATE_PROPERTY(int32_t, inventions_count);
		STATE_PROPERTY(Technology const*, current_research, nullptr);
		STATE_PROPERTY(fixed_point_t, invested_research_points);
		STATE_PROPERTY(fixed_point_t, current_research_cost);
		STATE_PROPERTY(Date, expected_research_completion_date);
		STATE_PROPERTY(fixed_point_t, research_point_stockpile);
		STATE_PROPERTY(fixed_point_t, daily_research_points);
		fixed_point_map_t<PopType const*> PROPERTY(research_points_from_pop_types);
		STATE_PROPERTY(TechnologySchool const*, tech_school, nullptr);
		// TODO - cached possible inventions with %age chance

		/* Politics */
		STATE_PROPERTY(NationalValue const*, national_value, nullptr);
		STATE_PROPERTY(GovernmentType const*, government_type, nullptr);
		Date PROPERTY(last_election);
		STATE_PROPERTY(CountryParty const*, ruling_party, nullptr);
		IndexedFlatMap_PROPERTY(Ideology, fixed_point_t, upper_house_proportion_by_ideology);
		IndexedFlatMap_PROPERTY(ReformGroup, Reform const*, reforms);
		STATE_PROPERTY(fixed_point_t, total_administrative_multiplier);
		RuleSet PROPERTY(rule_set);
		// TODO - national issue support distribution (for just voters and for everyone)
		IndexedFlatMap_PROPERTY(GovernmentType, GovernmentType const*, flag_overrides_by_government_type);
		STATE_PROPERTY(fixed_point_t, suppression_points);
		STATE_PROPERTY(fixed_point_t, infamy); // in 0-25+ range
		STATE_PROPERTY(fixed_point_t, plurality); // in 0-100 range
		STATE_PROPERTY(fixed_point_t, revanchism);
		IndexedFlatMap_PROPERTY(Crime, technology_unlock_level_t, crime_unlock_levels);
		// TODO - rebel movements

		/* Population */
		size_t PROPERTY(national_focus_capacity, 0);
		Culture const* PROPERTY(primary_culture, nullptr);
		ordered_set<Culture const*> PROPERTY(accepted_cultures);
		Religion const* PROPERTY(religion, nullptr);
		// TODO - population change over last 30 days

	public:
		DerivedState<GovernmentType const*> flag_government_type;
		DerivedState<fixed_point_t> total_score;
		DerivedState<fixed_point_t> military_power;
		DerivedState<fixed_point_t> research_progress;
		DerivedState<fixed_point_t> desired_administrator_percentage;
		DerivedState<fixed_point_t> corruption_cost_multiplier;
		DerivedState<fixed_point_t> tariff_efficiency;
		DerivedState<fixed_point_t> effective_tariff_rate;
		DerivedState<fixed_point_t> projected_administration_spending;
		DerivedState<fixed_point_t> projected_education_spending;
		DerivedState<fixed_point_t> projected_military_spending;
		DerivedState<fixed_point_t> projected_social_spending;
		DerivedState<fixed_point_t> projected_social_spending_unscaled_by_slider;
		DerivedState<fixed_point_t> projected_import_subsidies;
		DerivedState<fixed_point_t> projected_spending;
		DerivedState<bool> has_import_subsidies;
		
		fixed_point_t get_taxable_income_by_strata(Strata const& strata) const;
		// TODO - national foci

		/* Trade */
		struct good_data_t {
			memory::unique_ptr<std::mutex> mutex;
			fixed_point_t stockpile_amount;
			fixed_point_t stockpile_change_yesterday; // positive if we bought, negative if we sold

			bool is_automated = true;
			bool is_selling = false; // buying if false
			fixed_point_t stockpile_cutoff;

			fixed_point_t exported_amount; // negative if net importing

			fixed_point_t government_needs;
			fixed_point_t army_needs;
			fixed_point_t navy_needs;
			fixed_point_t overseas_maintenance;
			fixed_point_t factory_demand;
			fixed_point_t pop_demand;
			fixed_point_t available_amount;

			ordered_map<PopType const*, fixed_point_t> need_consumption_per_pop_type;
			ordered_map<ProductionType const*, fixed_point_t> input_consumption_per_production_type;
			ordered_map<ProductionType const*, fixed_point_t> production_per_production_type;

			good_data_t();
			good_data_t(good_data_t&&) = default;
			good_data_t& operator=(good_data_t&&) = default;

			//thread safe
			void clear_daily_recorded_data();
		};

	private:
		IndexedFlatMap_PROPERTY(GoodInstance, good_data_t, goods_data);

		/* Diplomacy */
		STATE_PROPERTY(fixed_point_t, prestige);
		size_t PROPERTY(prestige_rank, 0);
		fixed_point_t PROPERTY(diplomatic_points);
		// The last time this country lost a war, i.e. accepted a peace offer sent from their offer tab or the enemy's demand
		// tab, even white peace. Used for the "has_recently_lost_war" condition (true if the date is less than 5 years ago).
		Date PROPERTY(last_war_loss_date);
		vector_ordered_set<CountryInstance const*> PROPERTY(war_enemies);
		STATE_PROPERTY(CountryInstance const*, sphere_owner, nullptr);
		// TODO - colonial power, current wars

		/* Military */
		STATE_PROPERTY(fixed_point_t, military_power_from_land);
		STATE_PROPERTY(fixed_point_t, military_power_from_sea);
		STATE_PROPERTY(fixed_point_t, military_power_from_leaders);
		size_t PROPERTY(military_rank, 0);
		memory::vector<LeaderInstance*> SPAN_PROPERTY(generals);
		memory::vector<LeaderInstance*> SPAN_PROPERTY(admirals);
		memory::vector<ArmyInstance*> SPAN_PROPERTY(armies);
		memory::vector<NavyInstance*> SPAN_PROPERTY(navies);
		size_t PROPERTY(regiment_count, 0);
		size_t PROPERTY(mobilisation_potential_regiment_count, 0);
		size_t PROPERTY(mobilisation_max_regiment_count, 0);
		fixed_point_t PROPERTY(mobilisation_impact);
		fixed_point_t PROPERTY(mobilisation_economy_impact);
		fixed_point_t PROPERTY(supply_consumption);
		size_t PROPERTY(ship_count, 0);
		fixed_point_t PROPERTY(total_consumed_ship_supply);
		fixed_point_t PROPERTY(max_ship_supply);
		fixed_point_t PROPERTY_RW(leadership_point_stockpile);
		fixed_point_t PROPERTY(monthly_leadership_points);
		fixed_point_map_t<PopType const*> PROPERTY(leadership_points_from_pop_types);
		int32_t PROPERTY(create_leader_count, 0);
		// War exhaustion is stored as a raw decimal rather than a proportion, it is usually in the range 0-100.
		// The current war exhaustion value should only ever be modified via change_war_exhaustion(delta) which also
		// clamps the result in the range [0, war_exhaustion_max] straight away, matching the base game's behaviour.
		fixed_point_t PROPERTY(war_exhaustion);
		fixed_point_t PROPERTY(war_exhaustion_max);
		bool PROPERTY_RW_CUSTOM_NAME(mobilised, is_mobilised, set_mobilised, false);
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
		int32_t PROPERTY(dig_in_cap, 0);
		fixed_point_t PROPERTY(military_tactics);
		IndexedFlatMap_PROPERTY(RegimentType, technology_unlock_level_t, regiment_type_unlock_levels);
		regiment_allowed_cultures_t PROPERTY(allowed_regiment_cultures, regiment_allowed_cultures_t::NO_CULTURES);
		IndexedFlatMap_PROPERTY(ShipType, technology_unlock_level_t, ship_type_unlock_levels);
		technology_unlock_level_t PROPERTY(gas_attack_unlock_level, 0);
		technology_unlock_level_t PROPERTY(gas_defence_unlock_level, 0);
		memory::vector<technology_unlock_level_t> PROPERTY(unit_variant_unlock_levels);

	public:
		//pointers instead of references to allow construction via std::tuple
		CountryInstance(
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
		);
		CountryInstance(CountryInstance const&) = delete;
		CountryInstance& operator=(CountryInstance const&) = delete;
		CountryInstance(CountryInstance&&) = delete;
		CountryInstance& operator=(CountryInstance&&) = delete;

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		UNIT_BRANCHED_GETTER(get_unit_type_unlock_levels, regiment_type_unlock_levels, ship_type_unlock_levels);
		UNIT_BRANCHED_GETTER(get_leaders, generals, admirals);
		UNIT_BRANCHED_GETTER_CONST(get_leaders, generals, admirals);

		inline size_t get_general_count() const {
			return generals.size();
		}
		inline bool has_generals() const {
			return !generals.empty();
		}
		inline size_t get_admiral_count() const {
			return admirals.size();
		}
		inline bool has_admirals() const {
			return !admirals.empty();
		}
		inline size_t get_leader_count() const {
			return get_general_count() + get_admiral_count();
		}
		inline bool has_leaders() const {
			return has_generals() || has_admirals();
		}
		inline size_t get_army_count() const {
			return armies.size();
		}
		inline bool has_armies() const {
			return !armies.empty();
		}
		inline size_t get_navy_count() const {
			return navies.size();
		}
		inline bool has_navies() const {
			return !navies.empty();
		}

		std::string_view get_identifier() const;

		bool exists() const;
		bool is_civilised() const;
		bool can_colonise() const;
		bool is_great_power() const;
		bool is_secondary_power() const;
		bool is_at_war() const;
		bool is_neighbour(CountryInstance const& country) const;

		// Double-sided diplomacy functions

		CountryRelationManager::relation_value_type get_relations_with(CountryInstance const& country) const;
		void set_relations_with(CountryInstance& country, CountryRelationManager::relation_value_type relations);

		bool has_alliance_with(CountryInstance const& country) const;
		void set_alliance_with(CountryInstance& country, bool alliance = true);

		bool is_at_war_with(CountryInstance const& country) const;
		// Low-level setter function, should not be used to declare or join wars
		// Should generally be the basis for higher-level war functions
		void set_at_war_with(CountryInstance& country, bool at_war = true);

		// Single-sided diplomacy functions

		// Only detects military access diplomacy, do not use to validate troop movement
		// Prefer can_units_enter
		bool has_military_access_to(CountryInstance const& country) const;
		void set_military_access_to(CountryInstance& country, bool access = true);

		bool is_sending_war_subsidy_to(CountryInstance const& country) const;
		void set_sending_war_subsidy_to(CountryInstance& country, bool sending = true);

		bool is_commanding_units(CountryInstance const& country) const;
		void set_commanding_units(CountryInstance& country, bool commanding = true);

		bool has_vision_of(CountryInstance const& country) const;
		void set_has_vision_of(CountryInstance& country, bool vision = true);

		CountryRelationManager::OpinionType get_opinion_of(CountryInstance const& country) const;
		void set_opinion_of(CountryInstance& country, CountryRelationManager::OpinionType opinion);
		void increase_opinion_of(CountryInstance& country);
		void decrease_opinion_of(CountryInstance& country);

		CountryRelationManager::influence_value_type get_influence_with(CountryInstance const& country) const;
		void set_influence_with(CountryInstance& country, CountryRelationManager::influence_value_type influence);

		CountryRelationManager::influence_priority_value_type get_influence_priority_with(CountryInstance const& country) const;
		void set_influence_priority_with(CountryInstance& country, CountryRelationManager::influence_priority_value_type influence);

		std::optional<Date> get_decredited_from_date(CountryInstance const& country) const;
		void set_discredited_from(CountryInstance& country, Date until);

		std::optional<Date> get_embass_banned_from_date(CountryInstance const& country) const;
		void set_embassy_banned_from(CountryInstance& country, Date until);

		bool can_army_units_enter(CountryInstance const& country) const;
		bool can_navy_units_enter(CountryInstance const& country) const;

		// These functions take "std::string const&" rather than "std::string_view" as they're only used with script arguments
		// which are always stored as "std::string"s and it significantly simplifies mutable value access.
		fixed_point_t get_script_variable(memory::string const& variable_name) const;
		void set_script_variable(memory::string const& variable_name, fixed_point_t value);
		// Adds the argument value to the existing value of the script variable (initialised to 0 if it doesn't already exist).
		void change_script_variable(memory::string const& variable_name, fixed_point_t value);

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

		bool set_ruling_party(CountryParty const& new_ruling_party);
		bool add_reform(Reform const& new_reform);

		void set_strata_tax_rate_slider_value(Strata const& strata, const fixed_point_t new_value);
		void set_army_spending_slider_value(const fixed_point_t new_value);
		void set_navy_spending_slider_value(const fixed_point_t new_value);
		void set_construction_spending_slider_value(const fixed_point_t new_value);
		void set_education_spending_slider_value(const fixed_point_t new_value);
		void set_administration_spending_slider_value(const fixed_point_t new_value);
		void set_social_spending_slider_value(const fixed_point_t new_value);
		void set_military_spending_slider_value(const fixed_point_t new_value);
		void set_tariff_rate_slider_value(const fixed_point_t new_value);

		// Adds delta to the current war exhaustion value and clamps it to the range [0, war_exhaustion_max].
		void change_war_exhaustion(fixed_point_t delta);

		bool add_unit_instance_group(UnitInstanceGroup& group);
		bool remove_unit_instance_group(UnitInstanceGroup const& group);

		bool add_leader(LeaderInstance& leader);
		bool remove_leader(LeaderInstance const& leader);

		bool has_leader_with_name(std::string_view name) const;

		template<unit_branch_t Branch>
		bool modify_unit_type_unlock(UnitTypeBranched<Branch> const& unit_type, technology_unlock_level_t unlock_level_change);

		bool modify_unit_type_unlock(UnitType const& unit_type, technology_unlock_level_t unlock_level_change);
		bool unlock_unit_type(UnitType const& unit_type);
		bool is_unit_type_unlocked(UnitType const& unit_type) const;

		bool modify_building_type_unlock(
			BuildingType const& building_type, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
		);
		bool unlock_building_type(BuildingType const& building_type, GoodInstanceManager& good_instance_manager);
		bool is_building_type_unlocked(BuildingType const& building_type) const;

		bool modify_crime_unlock(Crime const& crime, technology_unlock_level_t unlock_level_change);
		bool unlock_crime(Crime const& crime);
		bool is_crime_unlocked(Crime const& crime) const;

		bool modify_gas_attack_unlock(technology_unlock_level_t unlock_level_change);
		bool unlock_gas_attack();
		bool is_gas_attack_unlocked() const;

		bool modify_gas_defence_unlock(technology_unlock_level_t unlock_level_change);
		bool unlock_gas_defence();
		bool is_gas_defence_unlocked() const;

		bool modify_unit_variant_unlock(unit_variant_t unit_variant, technology_unlock_level_t unlock_level_change);
		bool unlock_unit_variant(unit_variant_t unit_variant);
		unit_variant_t get_max_unlocked_unit_variant() const;

		bool modify_technology_unlock(
			Technology const& technology, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
		);
		bool set_technology_unlock_level(
			Technology const& technology, technology_unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
		);
		bool unlock_technology(Technology const& technology, GoodInstanceManager& good_instance_manager);
		bool is_technology_unlocked(Technology const& technology) const;

		bool modify_invention_unlock(
			Invention const& invention, technology_unlock_level_t unlock_level_change, GoodInstanceManager& good_instance_manager
		);
		bool set_invention_unlock_level(
			Invention const& invention, technology_unlock_level_t unlock_level, GoodInstanceManager& good_instance_manager
		);
		bool unlock_invention(Invention const& invention, GoodInstanceManager& good_instance_manager);
		bool is_invention_unlocked(Invention const& invention) const;

		bool is_primary_culture(Culture const& culture) const;
		// This only checks the accepted cultures list, ignoring the primary culture.
		bool is_accepted_culture(Culture const& culture) const;
		bool is_primary_or_accepted_culture(Culture const& culture) const;

		fixed_point_t calculate_research_cost(
			Technology const& technology, ModifierEffectCache const& modifier_effect_cache
		) const;
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

		//base here means not scaled by slider or pop size
		fixed_point_t calculate_pensions_base(
			ModifierEffectCache const& modifier_effect_cache,
			PopType const& pop_type
		);
		fixed_point_t calculate_unemployment_subsidies_base(
			ModifierEffectCache const& modifier_effect_cache,
			PopType const& pop_type
		);
		fixed_point_t calculate_minimum_wage_base(
			ModifierEffectCache const& modifier_effect_cache,
			PopType const& pop_type
		);

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
		void country_tick_before_map(InstanceManager& instance_manager);
		void country_tick_after_map(InstanceManager& instance_manager);

		good_data_t& get_good_data(GoodInstance const& good_instance);
		good_data_t const& get_good_data(GoodInstance const& good_instance) const;
		good_data_t& get_good_data(GoodDefinition const& good_definition);
		good_data_t const& get_good_data(GoodDefinition const& good_definition) const;

		//thread safe
		void report_pop_income_tax(PopType const& pop_type, const fixed_point_t gross_income, const fixed_point_t paid_as_tax);
		void report_pop_need_consumption(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_pop_need_demand(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_input_consumption(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_input_demand(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_output(ProductionType const& production_type, const fixed_point_t quantity);
		void request_salaries_and_welfare_and_import_subsidies(Pop& pop);
		fixed_point_t calculate_minimum_wage_base(PopType const& pop_type);
		fixed_point_t apply_tariff(const fixed_point_t money_spent_on_imports);
	};

	struct CountryDefinitionManager;
	struct CountryHistoryManager;

	struct CountryInstanceManager {
	private:
		CountryDefinitionManager const& PROPERTY(country_definition_manager);
		SharedCountryValues shared_country_values;

		IndexedFlatMap_PROPERTY(CountryDefinition, CountryInstance, country_instance_by_definition);

		memory::vector<CountryInstance*> SPAN_PROPERTY(great_powers);
		memory::vector<CountryInstance*> SPAN_PROPERTY(secondary_powers);

		memory::vector<CountryInstance*> SPAN_PROPERTY(total_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(prestige_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(industrial_power_ranking);
		memory::vector<CountryInstance*> SPAN_PROPERTY(military_power_ranking);

		void update_rankings(Date today, DefineManager const& define_manager);

	public:
		CountryInstanceManager(
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
		);

		constexpr OpenVic::utility::forwardable_span<CountryInstance> get_country_instances() {
			return country_instance_by_definition.get_values();
		}

		constexpr OpenVic::utility::forwardable_span<const CountryInstance> get_country_instances() const {
			return country_instance_by_definition.get_values();
		}
		CountryInstance* get_country_instance_by_identifier(std::string_view identifier);
		CountryInstance const* get_country_instance_by_identifier(std::string_view identifier) const;
		CountryInstance* get_country_instance_by_index(typename CountryInstance::index_t index);
		CountryInstance const* get_country_instance_by_index(typename CountryInstance::index_t index) const;
		CountryInstance& get_country_instance_by_definition(CountryDefinition const& country_definition); //const variant comes from IndexedFlatMap_PROPERTY

		bool apply_history_to_countries(InstanceManager& instance_manager);

		void update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(InstanceManager& instance_manager);
		void country_manager_tick_before_map(InstanceManager& instance_manager);
		void country_manager_tick_after_map(InstanceManager& instance_manager);
	};
}

extern template struct fmt::formatter<OpenVic::CountryInstance>;

#undef _UNIT_BRANCHED_GETTER
#undef UNIT_BRANCHED_GETTER
#undef UNIT_BRANCHED_GETTER_CONST
#undef IndexedFlatMap_PROPERTY
#undef IndexedFlatMap_PROPERTY_ACCESS
#undef CLAMPED_PROPERTY
#undef CLAMPED_PROPERTY_ACCESS