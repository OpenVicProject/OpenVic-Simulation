#pragma once

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	/* Representation of an existing country that is currently in-game. */
	struct CountryInstance {
	private:
		Country const* PROPERTY_RW(base_country);
		Culture const* PROPERTY_RW(primary_culture);
		std::vector<Culture const*> PROPERTY(accepted_cultures);
		Religion const* PROPERTY_RW(religion);
		CountryParty const* PROPERTY_RW(ruling_party);
		Date PROPERTY_RW(last_election);
		fixed_point_map_t<Ideology const*> PROPERTY(upper_house);
		Province const* PROPERTY_RW(capital);
		GovernmentType const* PROPERTY_RW(government_type);
		fixed_point_t PROPERTY_RW(plurality);
		NationalValue const* PROPERTY_RW(national_value);
		bool PROPERTY_RW(civilised);
		fixed_point_t PROPERTY_RW(prestige);
		std::vector<Reform const*> PROPERTY(reforms); // TODO: should be map of reform groups to active reforms: must set defaults & validate applied history
		// TODO: Military units + OOBs; will probably need an extensible deployment class

	public:
		bool add_accepted_culture(Culture const* new_accepted_culture);
		bool remove_accepted_culture(Culture const* culture_to_remove);
		/* Add or modify a party in the upper house. */
		void add_to_upper_house(Ideology const* party, fixed_point_t popularity);
		bool remove_from_upper_house(Ideology const* party);
		bool add_reform(Reform const* new_reform);
		bool remove_reform(Reform const* reform_to_remove);

		bool apply_history_to_country(CountryHistoryMap const& history, Date date);
	};
} // namespace OpenVic
