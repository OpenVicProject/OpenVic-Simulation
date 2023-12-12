#pragma once

#include <map>
#include <optional>

#include "openvic-simulation/misc/Decision.hpp"
#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
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
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"

namespace OpenVic {
	struct CountryHistoryMap;

	struct CountryHistoryEntry : HistoryEntry {
		friend struct CountryHistoryMap;

	private:
		Country const& PROPERTY(country);

		std::optional<Culture const*> PROPERTY(primary_culture);
		std::vector<Culture const*> PROPERTY(accepted_cultures);
		std::optional<Religion const*> PROPERTY(religion);
		std::optional<CountryParty const*> PROPERTY(ruling_party);
		std::optional<Date> PROPERTY(last_election);
		fixed_point_map_t<Ideology const*> PROPERTY(upper_house);
		std::optional<Province const*> PROPERTY(capital);
		std::optional<GovernmentType const*> PROPERTY(government_type);
		std::optional<fixed_point_t> PROPERTY(plurality);
		std::optional<NationalValue const*> PROPERTY(national_value);
		std::optional<bool> PROPERTY_CUSTOM_PREFIX(civilised, is);
		std::optional<fixed_point_t> PROPERTY(prestige);
		std::vector<Reform const*> PROPERTY(reforms);
		std::optional<Deployment const*> PROPERTY(inital_oob);
		std::optional<TechnologySchool const*> PROPERTY(tech_school);
		std::map<Technology const*, bool> PROPERTY(technologies);
		std::map<Invention const*, bool> PROPERTY(inventions);
		fixed_point_map_t<Country const*> PROPERTY(foreign_investment);
		std::optional<fixed_point_t> PROPERTY(consciousness);
		std::optional<fixed_point_t> PROPERTY(nonstate_consciousness);
		std::optional<fixed_point_t> PROPERTY(literacy);
		std::optional<fixed_point_t> PROPERTY(nonstate_culture_literacy);
		std::optional<bool> PROPERTY_CUSTOM_PREFIX(releasable_vassal, is);
		std::optional<fixed_point_t> PROPERTY(colonial_points);
		string_set_t PROPERTY(country_flags);
		string_set_t PROPERTY(global_flags);
		std::map<GovernmentType const*, GovernmentType const*> PROPERTY(government_flag_overrides);
		std::set<Decision const*> decisions;

		CountryHistoryEntry(Country const& new_country, Date new_date);
	};

	struct CountryHistoryManager;

	struct CountryHistoryMap : HistoryMap<CountryHistoryEntry, Dataloader const&, DeploymentManager&> {
		friend struct CountryHistoryManager;

	private:
		Country const& PROPERTY(country);

	protected:
		CountryHistoryMap(Country const& new_country);

		std::unique_ptr<CountryHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(
			GameManager const& game_manager, Dataloader const& dataloader, DeploymentManager& deployment_manager,
			CountryHistoryEntry& entry, ast::NodeCPtr root
		) override;
	};

	struct CountryHistoryManager {
	private:
		std::map<Country const*, CountryHistoryMap> country_histories;
		bool locked = false;

	public:
		CountryHistoryManager() = default;

		void lock_country_histories();
		bool is_locked() const;

		CountryHistoryMap const* get_country_history(Country const* country) const;

		bool load_country_history_file(
			GameManager& game_manager, Dataloader const& dataloader, Country const& country, ast::NodeCPtr root
		);
	};

} // namespace OpenVic
