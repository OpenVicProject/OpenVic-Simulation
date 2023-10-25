#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GameManager;
	struct CountryManager;

	struct CountryParty {
		friend struct CountryManager;

	private:
		const std::string name;
		const Date start_date;
		const Date end_date;
		const Ideology& ideology;
		const std::vector<Issue const*> policies;

		CountryParty(
			std::string_view new_name,
			Date new_start_date,
			Date new_end_date,
			const Ideology& new_ideology,
			std::vector<const Issue*>&& new_policies
		);

	public:
		std::string_view get_name() const;
		const Date& get_start_date() const;
		const Date& get_end_date() const;
		const Ideology& get_ideology() const;
		const std::vector<const Issue*>& get_policies() const;
	};

	struct UnitNames {
		friend struct CountryManager;

	private:
		const std::string identifier;
		const std::vector<std::string> names;

		UnitNames(std::string_view new_identifier, std::vector<std::string>&& new_names);

	public:
		std::string_view get_identifier() const;
		const std::vector<std::string>& get_names() const;
	};

	struct Country : HasIdentifierAndColour {
		friend struct CountryManager;

	private:
		const GraphicalCultureType& graphical_culture;
		const std::vector<CountryParty> parties;
		const std::vector<UnitNames> unit_names;
		const bool dynamic_tag;
		const std::map<const GovernmentType*, colour_t> alternative_colours;

		Country(
			std::string_view new_identifier,
			colour_t new_color,
			const GraphicalCultureType& new_graphical_culture,
			std::vector<CountryParty>&& new_parties,
			std::vector<UnitNames>&& new_unit_names,
			const bool new_dynamic_tag,
			std::map<const GovernmentType*, colour_t>&& new_alternative_colours
		);

	public:
		const GraphicalCultureType& get_graphical_culture() const;
		const std::vector<CountryParty>& get_parties() const;
		const std::vector<UnitNames>& get_unit_names() const;
		const bool is_dynamic_tag() const;
		const std::map<const GovernmentType*, colour_t>& get_alternative_colours() const;
	};

	struct CountryManager {
	private:
		IdentifierRegistry<Country> countries;

	public:
		CountryManager();

		bool add_country(
			std::string_view identifier,
			colour_t color,
			const GraphicalCultureType& graphical_culture,
			std::vector<CountryParty>&& parties,
			std::vector<UnitNames>&& unit_names,
			bool dynamic_tag,
			std::map<const GovernmentType*, colour_t>&& alternative_colours
		);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(country, countries);

		bool load_country_data_file(GameManager& game_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root);
	};
}