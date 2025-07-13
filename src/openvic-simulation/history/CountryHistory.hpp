#pragma once

#include <optional>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct CountryHistoryMap;
	struct CountryDefinition;
	struct Culture;
	struct Religion;
	struct CountryParty;
	struct Ideology;
	struct ProvinceDefinition;
	struct GovernmentType;
	struct NationalValue;
	struct Reform;
	struct Deployment;
	struct TechnologySchool;
	struct Technology;
	struct Invention;
	struct Decision;

	struct CountryHistoryEntry : HistoryEntry {
		friend struct CountryHistoryMap;
	private:
		CountryDefinition const& PROPERTY(country);

		std::optional<Culture const*> PROPERTY(primary_culture);
		ordered_map<Culture const*, bool> PROPERTY(accepted_cultures);
		std::optional<Religion const*> PROPERTY(religion);
		std::optional<CountryParty const*> PROPERTY(ruling_party);
		std::optional<Date> PROPERTY(last_election);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(upper_house);
		std::optional<ProvinceDefinition const*> PROPERTY(capital);
		std::optional<GovernmentType const*> PROPERTY(government_type);
		std::optional<fixed_point_t> PROPERTY(plurality);
		std::optional<NationalValue const*> PROPERTY(national_value);
		std::optional<bool> PROPERTY_CUSTOM_PREFIX(civilised, is);
		std::optional<fixed_point_t> PROPERTY(prestige);
		ordered_set<Reform const*> PROPERTY(reforms);
		std::optional<Deployment const*> PROPERTY(initial_oob);
		std::optional<TechnologySchool const*> PROPERTY(tech_school);
		ordered_map<Technology const*, CountryInstance::unlock_level_t> PROPERTY(technologies);
		ordered_map<Invention const*, bool> PROPERTY(inventions);
		IndexedMap<CountryDefinition, fixed_point_t> PROPERTY(foreign_investment);
		std::optional<fixed_point_t> PROPERTY(consciousness);
		std::optional<fixed_point_t> PROPERTY(nonstate_consciousness);
		std::optional<fixed_point_t> PROPERTY(literacy);
		std::optional<fixed_point_t> PROPERTY(nonstate_culture_literacy);
		std::optional<bool> PROPERTY_CUSTOM_PREFIX(releasable_vassal, is);
		std::optional<fixed_point_t> PROPERTY(colonial_points);
		// True for set, false for clear
		string_map_t<bool> PROPERTY(country_flags);
		string_map_t<bool> PROPERTY(global_flags);
		IndexedMap<GovernmentType, GovernmentType const*> PROPERTY(government_flag_overrides);
		ordered_set<Decision const*> PROPERTY(decisions);

	public:
		CountryHistoryEntry(
			CountryDefinition const& new_country, Date new_date, decltype(upper_house)::keys_span_type ideology_keys,
			decltype(government_flag_overrides)::keys_span_type government_type_keys
		);
	};

	class Dataloader;
	struct DeploymentManager;
	struct CountryHistoryManager;

	struct CountryHistoryMap : HistoryMap<CountryHistoryEntry, Dataloader const&, DeploymentManager&> {
		friend struct CountryHistoryManager;

	private:
		CountryDefinition const& PROPERTY(country);
		decltype(CountryHistoryEntry::upper_house)::keys_span_type PROPERTY(ideology_keys);
		decltype(CountryHistoryEntry::government_flag_overrides)::keys_span_type PROPERTY(government_type_keys);

	protected:
		CountryHistoryMap(
			CountryDefinition const& new_country, decltype(ideology_keys) new_ideology_keys,
			decltype(government_type_keys) new_government_type_keys
		);

		memory::unique_ptr<CountryHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(
			DefinitionManager const& definition_manager, Dataloader const& dataloader, DeploymentManager& deployment_manager,
			CountryHistoryEntry& entry, ast::NodeCPtr root
		) override;
	};

	struct CountryHistoryManager {
	private:
		ordered_map<CountryDefinition const*, CountryHistoryMap> country_histories;
		bool locked = false;

	public:
		CountryHistoryManager() = default;

		void reserve_more_country_histories(size_t size);
		void lock_country_histories();
		bool is_locked() const;

		CountryHistoryMap const* get_country_history(CountryDefinition const* country) const;

		bool load_country_history_file(
			DefinitionManager& definition_manager, Dataloader const& dataloader, CountryDefinition const& country,
			decltype(CountryHistoryMap::ideology_keys) ideology_keys,
			decltype(CountryHistoryMap::government_type_keys) government_type_keys, ast::NodeCPtr root
		);
	};

}
