#pragma once

#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/politics/Rule.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include "openvic-simulation/ModifierCalculationTestToggle.hpp"

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
	struct MapInstance;
	struct DefineManager;
	struct ModifierEffectCache;
	struct StaticModifierCache;

	/* Representation of a country's mutable attributes, with a CountryDefinition that is unique at any single time
	 * but can be swapped with other CountryInstance's CountryDefinition when switching tags. */
	struct CountryInstance {
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

		using unlock_level_t = int8_t;
		using unit_variant_t = uint8_t;

	private:
#if OV_MODIFIER_CALCULATION_TEST
		const bool ADD_OWNER_CONTRIBUTION;
#endif

		/* Main attributes */
		// We can always assume country_definition is not null, as it is initialised from a reference and only ever changed
		// by swapping with another CountryInstance's country_definition.
		CountryDefinition const* PROPERTY(country_definition);
		colour_t PROPERTY(colour); // Cached to avoid searching government overrides for every province
		ProvinceInstance const* PROPERTY(capital);
		string_set_t PROPERTY(country_flags);
		bool PROPERTY_CUSTOM_PREFIX(releasable_vassal, is);

		country_status_t PROPERTY(country_status);
		Date PROPERTY(lose_great_power_date);
		fixed_point_t PROPERTY(total_score);
		size_t PROPERTY(total_rank);

		ordered_set<ProvinceInstance*> PROPERTY(owned_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(controlled_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(core_provinces);
		ordered_set<State*> PROPERTY(states);

		// The total/resultant modifier affecting this country, including owned province contributions.
		ModifierSum PROPERTY(modifier_sum);
		std::vector<ModifierInstance> PROPERTY(event_modifiers);

		/* Production */
		fixed_point_t PROPERTY(industrial_power);
		std::vector<std::pair<State const*, fixed_point_t>> PROPERTY(industrial_power_from_states);
		std::vector<std::pair<CountryInstance const*, fixed_point_t>> PROPERTY(industrial_power_from_investments);
		size_t PROPERTY(industrial_rank);
		fixed_point_map_t<CountryInstance const*> PROPERTY(foreign_investments);
		IndexedMap<BuildingType, unlock_level_t> PROPERTY(building_type_unlock_levels);
		// TODO - total amount of each good produced

		/* Budget */
		fixed_point_t PROPERTY(cash_stockpile);
		// TODO - cash stockpile change over last 30 days

		/* Technology */
		IndexedMap<Technology, unlock_level_t> PROPERTY(technology_unlock_levels);
		IndexedMap<Invention, unlock_level_t> PROPERTY(invention_unlock_levels);
		Technology const* PROPERTY(current_research);
		fixed_point_t PROPERTY(invested_research_points);
		Date PROPERTY(expected_completion_date);
		fixed_point_t PROPERTY(research_point_stockpile);
		fixed_point_t PROPERTY(daily_research_points); // TODO - breakdown by source
		fixed_point_t PROPERTY(national_literacy);
		TechnologySchool const* PROPERTY(tech_school);
		// TODO - cached possible inventions with %age chance

		/* Politics */
		NationalValue const* PROPERTY(national_value);
		GovernmentType const* PROPERTY(government_type);
		Date PROPERTY(last_election);
		CountryParty const* PROPERTY(ruling_party);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(upper_house);
		IndexedMap<ReformGroup, Reform const*> PROPERTY(reforms);
		fixed_point_t PROPERTY(total_administrative_multiplier);
		RuleSet PROPERTY(rule_set);
		// TODO - national issue support distribution (for just voters and for everyone)
		IndexedMap<GovernmentType, GovernmentType const*> PROPERTY(government_flag_overrides);
		GovernmentType const* PROPERTY(flag_government_type);
		fixed_point_t PROPERTY(suppression_points);
		fixed_point_t PROPERTY(infamy); // in 0-25+ range
		fixed_point_t PROPERTY(plurality); // in 0-100 range
		fixed_point_t PROPERTY(revanchism);
		IndexedMap<Crime, unlock_level_t> PROPERTY(crime_unlock_levels);
		// TODO - rebel movements

		/* Population */
		Culture const* PROPERTY(primary_culture);
		ordered_set<Culture const*> PROPERTY(accepted_cultures);
		Religion const* PROPERTY(religion);
		Pop::pop_size_t PROPERTY(total_population);
		// TODO - population change over last 30 days
		fixed_point_t PROPERTY(national_consciousness);
		fixed_point_t PROPERTY(national_militancy);
		IndexedMap<PopType, fixed_point_t> PROPERTY(pop_type_distribution);
		size_t PROPERTY(national_focus_capacity)
		// TODO - national foci

		/* Trade */
		// TODO - total amount of each good exported and imported

		/* Diplomacy */
		fixed_point_t PROPERTY(prestige);
		size_t PROPERTY(prestige_rank);
		fixed_point_t PROPERTY(diplomatic_points);
		// TODO - colonial power, current wars

		/* Military */
		fixed_point_t PROPERTY(military_power);
		fixed_point_t PROPERTY(military_power_from_land);
		fixed_point_t PROPERTY(military_power_from_sea);
		fixed_point_t PROPERTY(military_power_from_leaders);
		size_t PROPERTY(military_rank);
		plf::colony<General> PROPERTY(generals);
		plf::colony<Admiral> PROPERTY(admirals);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);
		size_t PROPERTY(regiment_count);
		size_t PROPERTY(max_supported_regiment_count);
		size_t PROPERTY(mobilisation_potential_regiment_count);
		size_t PROPERTY(mobilisation_max_regiment_count);
		fixed_point_t PROPERTY(mobilisation_impact);
		fixed_point_t PROPERTY(supply_consumption);
		size_t PROPERTY(ship_count);
		fixed_point_t PROPERTY(total_consumed_ship_supply);
		fixed_point_t PROPERTY(max_ship_supply);
		fixed_point_t PROPERTY(leadership_points);
		fixed_point_t PROPERTY(war_exhaustion); // in 0-100 range
		bool PROPERTY_CUSTOM_PREFIX(mobilised, is);
		bool PROPERTY_CUSTOM_PREFIX(disarmed, is);
		IndexedMap<RegimentType, unlock_level_t> PROPERTY(regiment_type_unlock_levels);
		RegimentType::allowed_cultures_t PROPERTY(allowed_regiment_cultures);
		IndexedMap<ShipType, unlock_level_t> PROPERTY(ship_type_unlock_levels);
		unlock_level_t PROPERTY(gas_attack_unlock_level);
		unlock_level_t PROPERTY(gas_defence_unlock_level);
		std::vector<unlock_level_t> PROPERTY(unit_variant_unlock_levels);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		UNIT_BRANCHED_GETTER(get_leaders, generals, admirals);
		UNIT_BRANCHED_GETTER(get_unit_type_unlock_levels, regiment_type_unlock_levels, ship_type_unlock_levels);

		CountryInstance(
#if OV_MODIFIER_CALCULATION_TEST
			bool new_ADD_OWNER_CONTRIBUTION,
#endif
			CountryDefinition const* new_country_definition,
			decltype(building_type_unlock_levels)::keys_t const& building_type_keys,
			decltype(technology_unlock_levels)::keys_t const& technology_keys,
			decltype(invention_unlock_levels)::keys_t const& invention_keys,
			decltype(upper_house)::keys_t const& ideology_keys,
			decltype(reforms)::keys_t const& reform_keys,
			decltype(government_flag_overrides)::keys_t const& government_type_keys,
			decltype(crime_unlock_levels)::keys_t const& crime_keys,
			decltype(pop_type_distribution)::keys_t const& pop_type_keys,
			decltype(regiment_type_unlock_levels)::keys_t const& regiment_type_unlock_levels_keys,
			decltype(ship_type_unlock_levels)::keys_t const& ship_type_unlock_levels_keys
		);

	public:
		std::string_view get_identifier() const;

		bool exists() const;
		bool is_civilised() const;
		bool can_colonise() const;
		bool is_great_power() const;
		bool is_secondary_power() const;

		bool set_country_flag(std::string_view flag, bool warn);
		bool clear_country_flag(std::string_view flag, bool warn);
		bool add_owned_province(ProvinceInstance& new_province);
		bool remove_owned_province(ProvinceInstance& province_to_remove);
		bool add_controlled_province(ProvinceInstance& new_province);
		bool remove_controlled_province(ProvinceInstance& province_to_remove);
		bool add_core_province(ProvinceInstance& new_core);
		bool remove_core_province(ProvinceInstance& core_to_remove);
		bool add_state(State& new_state);
		bool remove_state(State& state_to_remove);

		bool add_accepted_culture(Culture const& new_accepted_culture);
		bool remove_accepted_culture(Culture const& culture_to_remove);
		/* Set a party's popularity in the upper house. */
		bool set_upper_house(Ideology const* ideology, fixed_point_t popularity);
		bool set_ruling_party(CountryParty const& new_ruling_party);
		bool add_reform(Reform const& new_reform);

		template<UnitType::branch_t Branch>
		bool add_unit_instance_group(UnitInstanceGroup<Branch>& group);
		template<UnitType::branch_t Branch>
		bool remove_unit_instance_group(UnitInstanceGroup<Branch>& group);

		template<UnitType::branch_t Branch>
		void add_leader(LeaderBranched<Branch>&& leader);
		template<UnitType::branch_t Branch>
		bool remove_leader(LeaderBranched<Branch> const* leader);

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

		// Sets the investment of each country in the map (rather than adding to them), leaving the rest unchanged.
		void apply_foreign_investments(
			fixed_point_map_t<CountryDefinition const*> const& investments,
			CountryInstanceManager const& country_instance_manager
		);

		bool apply_history_to_country(
			CountryHistoryEntry const& entry, MapInstance& map_instance, CountryInstanceManager const& country_instance_manager
		);

	private:
		void _update_production(DefineManager const& define_manager);
		void _update_budget();
		void _update_technology();
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
		fixed_point_t get_modifier_effect_value_nullcheck(ModifierEffect const* effect) const;
		void push_contributing_modifiers(
			ModifierEffect const& effect, std::vector<ModifierSum::modifier_entry_t>& contributions
		) const;
		std::vector<ModifierSum::modifier_entry_t> get_contributing_modifiers(ModifierEffect const& effect) const;

		void update_gamestate(
			DefineManager const& define_manager, UnitTypeManager const& unit_type_manager,
			ModifierEffectCache const& modifier_effect_cache
		);
		void tick();
	};

	struct CountryDefinitionManager;
	struct CountryHistoryManager;
	struct UnitInstanceManager;

	struct CountryInstanceManager {
	private:
		IdentifierRegistry<CountryInstance> IDENTIFIER_REGISTRY(country_instance);

		std::vector<CountryInstance*> PROPERTY(great_powers);
		std::vector<CountryInstance*> PROPERTY(secondary_powers);

		std::vector<CountryInstance*> PROPERTY(total_ranking);
		std::vector<CountryInstance*> PROPERTY(prestige_ranking);
		std::vector<CountryInstance*> PROPERTY(industrial_power_ranking);
		std::vector<CountryInstance*> PROPERTY(military_power_ranking);

		void update_rankings(Date today, DefineManager const& define_manager);

	public:
		CountryInstance& get_country_instance_from_definition(CountryDefinition const& country);
		CountryInstance const& get_country_instance_from_definition(CountryDefinition const& country) const;

		bool generate_country_instances(
#if OV_MODIFIER_CALCULATION_TEST
			bool ADD_OWNER_CONTRIBUTION,
#endif
			CountryDefinitionManager const& country_definition_manager,
			decltype(CountryInstance::building_type_unlock_levels)::keys_t const& building_type_keys,
			decltype(CountryInstance::technology_unlock_levels)::keys_t const& technology_keys,
			decltype(CountryInstance::invention_unlock_levels)::keys_t const& invention_keys,
			decltype(CountryInstance::upper_house)::keys_t const& ideology_keys,
			decltype(CountryInstance::reforms)::keys_t const& reform_keys,
			decltype(CountryInstance::government_flag_overrides)::keys_t const& government_type_keys,
			decltype(CountryInstance::crime_unlock_levels)::keys_t const& crime_keys,
			decltype(CountryInstance::pop_type_distribution)::keys_t const& pop_type_keys,
			decltype(CountryInstance::regiment_type_unlock_levels)::keys_t const& regiment_type_unlock_levels_keys,
			decltype(CountryInstance::ship_type_unlock_levels)::keys_t const& ship_type_unlock_levels_keys
		);

		bool apply_history_to_countries(
			CountryHistoryManager const& history_manager, Date date, UnitInstanceManager& unit_instance_manager,
			MapInstance& map_instance
		);

		void update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(
			Date today, DefineManager const& define_manager, UnitTypeManager const& unit_type_manager,
			ModifierEffectCache const& modifier_effect_cache
		);
		void tick();
	};
}
