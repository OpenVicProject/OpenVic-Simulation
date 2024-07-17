#pragma once

#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/military/Leader.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct Culture;
	struct Religion;
	struct CountryParty;
	struct Ideology;
	struct ProvinceDefinition;
	struct GovernmentType;
	struct NationalValue;
	struct Reform;
	struct CountryHistoryEntry;

	/* Representation of an existing country that is currently in-game. */
	struct CountryInstance {
		friend struct CountryInstanceManager;

	private:
		CountryDefinition const* PROPERTY_RW(country_definition);
		Culture const* PROPERTY_RW(primary_culture);
		std::vector<Culture const*> PROPERTY(accepted_cultures);
		Religion const* PROPERTY_RW(religion);
		CountryParty const* PROPERTY_RW(ruling_party);
		Date PROPERTY_RW(last_election);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(upper_house);
		// TODO - should this be ProvinceInstance and/or non-const pointer?
		// Currently ProvinceDefinition as that's what CountryHistoryEntry has (loaded prior to ProvinceInstance generation)
		ProvinceDefinition const* PROPERTY_RW(capital);
		GovernmentType const* PROPERTY_RW(government_type);
		fixed_point_t PROPERTY_RW(plurality);
		NationalValue const* PROPERTY_RW(national_value);
		bool PROPERTY_RW(civilised);
		fixed_point_t PROPERTY_RW(prestige);
		std::vector<Reform const*> PROPERTY(reforms); // TODO: should be map of reform groups to active reforms: must set defaults & validate applied history

		IndexedMap<Technology, bool> PROPERTY(technologies);
		IndexedMap<Invention, bool> PROPERTY(inventions);

		// TODO: Military units + OOBs; will probably need an extensible deployment class

		plf::colony<General> PROPERTY(generals);
		plf::colony<Admiral> PROPERTY(admirals);

		CountryInstance(
			CountryDefinition const* new_country_definition, decltype(technologies)::keys_t const& technology_keys,
			decltype(inventions)::keys_t const& invention_keys, decltype(upper_house)::keys_t const& ideology_keys
		);

	public:
		std::string_view get_identifier() const;

		bool add_accepted_culture(Culture const* new_accepted_culture);
		bool remove_accepted_culture(Culture const* culture_to_remove);
		/* Set a party's popularity in the upper house. */
		bool set_upper_house(Ideology const* ideology, fixed_point_t popularity);
		bool add_reform(Reform const* new_reform);
		bool remove_reform(Reform const* reform_to_remove);

		void add_general(General&& new_general);
		bool remove_general(General const* general_to_remove);
		void add_admiral(Admiral&& new_admiral);
		bool remove_admiral(Admiral const* admiral_to_remove);

		bool add_leader(LeaderBase const& new_leader);
		bool remove_leader(LeaderBase const* leader_to_remove);

		bool apply_history_to_country(CountryHistoryEntry const* entry);
	};

	struct CountryDefinitionManager;
	struct CountryHistoryManager;
	struct UnitInstanceManager;
	struct MapInstance;

	struct CountryInstanceManager {
	private:
		IdentifierRegistry<CountryInstance> IDENTIFIER_REGISTRY(country_instance);

	public:
		bool generate_country_instances(
			CountryDefinitionManager const& country_definition_manager,
			decltype(CountryInstance::technologies)::keys_t const& technology_keys,
			decltype(CountryInstance::inventions)::keys_t const& invention_keys,
			decltype(CountryInstance::upper_house)::keys_t const& ideology_keys
		);

		bool apply_history_to_countries(
			CountryHistoryManager const& history_manager, Date date, UnitInstanceManager& unit_instance_manager,
			MapInstance& map_instance
		);
	};
}
