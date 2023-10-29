#pragma once

#include <map>
#include <vector>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {
	struct CountryHistoryManager;

	struct CountryHistory {
		friend struct CountryHistoryManager;

	private:
		Culture const* primary_culture;
		std::vector<Culture const*> accepted_cultures;
		Religion const* religion;
		CountryParty const* ruling_party;
		Date last_election;
		std::map<Ideology const*, fixed_point_t> upper_house;
		Province const* capital;
		GovernmentType const* government_type;
		fixed_point_t plurality;
		NationalValue const* national_value;
		bool civilised;
		fixed_point_t prestige;
		std::vector<Reform const*> reforms;
		Deployment const* inital_oob;
		// TODO: technologies, tech schools, and inventions when PR#51 merged
		// TODO: starting foreign investment

		CountryHistory(
			Culture const* new_primary_culture, std::vector<Culture const*>&& new_accepted_cultures,
			Religion const* new_religion, CountryParty const* new_ruling_party, Date new_last_election,
			std::map<Ideology const*, fixed_point_t>&& new_upper_house, Province const* new_capital,
			GovernmentType const* new_government_type, fixed_point_t new_plurality, NationalValue const* new_national_value,
			bool new_civilised, fixed_point_t new_prestige, std::vector<Reform const*>&& new_reforms,
			Deployment const* new_inital_oob
		);

	public:
		Culture const* get_primary_culture() const;
		const std::vector<Culture const*>& get_accepted_cultures() const;
		Religion const* get_religion() const;
		CountryParty const* get_ruling_party() const;
		const Date get_last_election() const;
		const std::map<Ideology const*, fixed_point_t>& get_upper_house() const;
		Province const* get_capital() const;
		GovernmentType const* get_government_type() const;
		const fixed_point_t get_plurality() const;
		NationalValue const* get_national_value() const;
		const bool is_civilised() const;
		const fixed_point_t get_prestige() const;
		const std::vector<Reform const*>& get_reforms() const;
		Deployment const* get_inital_oob() const;
	};

	struct CountryHistoryManager {
	private:
		std::map<Country const*, std::map<Date, CountryHistory>> country_histories;
		bool locked = false;

		inline bool _load_country_history_entry(
			GameManager& game_manager, std::string_view name, Date const& date, ast::NodeCPtr root
		);

	public:
		CountryHistoryManager() {}

		bool add_country_history_entry(
			Country const* country, Date date, Culture const* primary_culture, std::vector<Culture const*>&& accepted_cultures,
			Religion const* religion, CountryParty const* ruling_party, Date last_election,
			std::map<Ideology const*, fixed_point_t>&& upper_house, Province const* capital,
			GovernmentType const* government_type, fixed_point_t plurality, NationalValue const* national_value, bool civilised,
			fixed_point_t prestige, std::vector<Reform const*>&& reforms, Deployment const* initial_oob,
			bool updated_accepted_cultures, bool updated_upper_house, bool updated_reforms
		);

		void lock_country_histories();
		bool is_locked() const;

		/* Returns history of country at date, if date doesn't have an entry returns
		 * closest entry before date. Return can be nullptr if an error occurs. */
		CountryHistory const* get_country_history(Country const* country, Date entry) const;
		/* Returns history of country at bookmark date. Return can be nullptr if an error occurs. */
		inline CountryHistory const* get_country_history(Country const* country, Bookmark const* entry) const;

		bool load_country_history_file(GameManager& game_manager, std::string_view name, ast::NodeCPtr root);
	};

} // namespace OpenVic
