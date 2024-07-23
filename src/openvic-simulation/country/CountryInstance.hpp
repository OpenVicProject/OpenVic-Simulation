#pragma once

#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
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
	struct Reform;
	struct Culture;
	struct Religion;
	struct CountryHistoryEntry;
	struct MapInstance;

	/* Representation of a country's mutable attributes, with a CountryDefinition that is unique at any single time
	 * but can be swapped with other CountryInstance's CountryDefinition when switching tags. */
	struct CountryInstance {
		friend struct CountryInstanceManager;

	private:
		/* Main attributes */
		// We can always assume country_definition is not null, as it is initialised from a reference and only ever changed
		// by swapping with another CountryInstance's country_definition.
		CountryDefinition const* PROPERTY(country_definition);
		colour_t PROPERTY(colour); // Cached to avoid searching government overrides for every province
		ProvinceInstance const* PROPERTY(capital);
		string_set_t PROPERTY(country_flags);

		bool PROPERTY(civilised);
		bool PROPERTY_CUSTOM_PREFIX(releasable_vassal, is);

		ordered_set<ProvinceInstance*> PROPERTY(owned_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(controlled_provinces);
		ordered_set<ProvinceInstance*> PROPERTY(core_provinces);
		ordered_set<State*> PROPERTY(states);

		/* Production */
		// TODO - total amount of each good produced

		/* Budget */
		fixed_point_t PROPERTY(cash_stockpile);
		// TODO - cash stockpile change over last 30 days

		/* Technology */
		IndexedMap<Technology, bool> PROPERTY(technologies);
		IndexedMap<Invention, bool> PROPERTY(inventions);
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
		std::vector<Reform const*> PROPERTY(reforms); // TODO: should be map of reform groups to active reforms: must set defaults & validate applied history
		// TODO - national issue support distribution (for just voters and for everyone)
		fixed_point_t PROPERTY(suppression_points);
		fixed_point_t PROPERTY(infamy);
		fixed_point_t PROPERTY(plurality);
		fixed_point_t PROPERTY(revanchism);
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
		size_t PROPERTY(total_rank);
		fixed_point_t PROPERTY(prestige);
		size_t PROPERTY(prestige_rank);
		fixed_point_t PROPERTY(industrial_power);
		size_t PROPERTY(industrial_rank);
		fixed_point_t PROPERTY(military_power);
		size_t PROPERTY(military_rank);
		fixed_point_t PROPERTY(diplomatic_points);
		// TODO - colonial power, current wars

		/* Military */
		plf::colony<General> PROPERTY(generals);
		plf::colony<Admiral> PROPERTY(admirals);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);
		size_t PROPERTY(regiment_count);
		size_t PROPERTY(mobilisation_regiment_potential);
		size_t PROPERTY(ship_count);
		fixed_point_t PROPERTY(total_consumed_ship_supply);
		fixed_point_t PROPERTY(max_ship_supply);
		fixed_point_t PROPERTY(leadership_points);
		fixed_point_t PROPERTY(war_exhaustion);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		UNIT_BRANCHED_GETTER(get_leaders, generals, admirals);

		CountryInstance(
			CountryDefinition const* new_country_definition, decltype(technologies)::keys_t const& technology_keys,
			decltype(inventions)::keys_t const& invention_keys, decltype(upper_house)::keys_t const& ideology_keys,
			decltype(pop_type_distribution)::keys_t const& pop_type_keys
		);

	public:
		std::string_view get_identifier() const;

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
		bool add_reform(Reform const* new_reform);
		bool remove_reform(Reform const* reform_to_remove);

		template<UnitType::branch_t Branch>
		bool add_unit_instance_group(UnitInstanceGroup<Branch>& group);
		template<UnitType::branch_t Branch>
		bool remove_unit_instance_group(UnitInstanceGroup<Branch>& group);

		template<UnitType::branch_t Branch>
		void add_leader(LeaderBranched<Branch>&& leader);
		template<UnitType::branch_t Branch>
		bool remove_leader(LeaderBranched<Branch> const* leader);

		bool apply_history_to_country(CountryHistoryEntry const* entry, MapInstance& map_instance);

	private:
		void _update_production();
		void _update_budget();
		void _update_technology();
		void _update_politics();
		void _update_population();
		void _update_trade();
		void _update_diplomacy();
		void _update_military();

	public:

		void update_gamestate();
		void tick();
	};

	struct CountryDefinitionManager;
	struct CountryHistoryManager;
	struct UnitInstanceManager;

	struct CountryInstanceManager {
	private:
		IdentifierRegistry<CountryInstance> IDENTIFIER_REGISTRY(country_instance);

	public:
		CountryInstance& get_country_instance_from_definition(CountryDefinition const& country);
		CountryInstance const& get_country_instance_from_definition(CountryDefinition const& country) const;

		bool generate_country_instances(
			CountryDefinitionManager const& country_definition_manager,
			decltype(CountryInstance::technologies)::keys_t const& technology_keys,
			decltype(CountryInstance::inventions)::keys_t const& invention_keys,
			decltype(CountryInstance::upper_house)::keys_t const& ideology_keys,
			decltype(CountryInstance::pop_type_distribution)::keys_t const& pop_type_keys
		);

		bool apply_history_to_countries(
			CountryHistoryManager const& history_manager, Date date, UnitInstanceManager& unit_instance_manager,
			MapInstance& map_instance
		);

		void update_gamestate();
		void tick();
	};
}
