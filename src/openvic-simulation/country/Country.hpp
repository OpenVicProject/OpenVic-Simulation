#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GameManager;
	struct CountryManager;

	struct CountryParty : HasIdentifier {
		friend struct CountryManager;

		using policy_map_t = std::map<IssueGroup const*, Issue const*>;

	private:
		const Date start_date;
		const Date end_date;
		Ideology const& ideology;
		const policy_map_t policies;

		CountryParty(
			std::string_view new_identifier, Date new_start_date, Date new_end_date, Ideology const& new_ideology,
			policy_map_t&& new_policies
		);

	public:
		CountryParty(CountryParty&&) = default;

		Date get_start_date() const;
		Date get_end_date() const;
		Ideology const& get_ideology() const;
		policy_map_t const& get_policies() const;
	};

	struct Country : HasIdentifierAndColour {
		friend struct CountryManager;

		using unit_names_map_t = std::map<Unit const*, std::vector<std::string>>;
		using government_colour_map_t = std::map<GovernmentType const*, colour_t>;

	private:
		GraphicalCultureType const& graphical_culture;
		/* Not const to allow elements to be moved, otherwise a copy is forced
		 * which causes a compile error as the copy constructor has been deleted.
		 */
		IdentifierRegistry<CountryParty> parties;
		const unit_names_map_t unit_names;
		const bool dynamic_tag;
		const government_colour_map_t alternative_colours;

		Country(
			std::string_view new_identifier, colour_t new_colour, GraphicalCultureType const& new_graphical_culture,
			IdentifierRegistry<CountryParty>&& new_parties, unit_names_map_t&& new_unit_names, bool new_dynamic_tag,
			government_colour_map_t&& new_alternative_colours
		);

	public:
		Country(Country&&) = default;

		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(party, parties)

		GraphicalCultureType const& get_graphical_culture() const;
		unit_names_map_t const& get_unit_names() const;
		bool is_dynamic_tag() const;
		government_colour_map_t const& get_alternative_colours() const;
	};

	struct CountryManager {
	private:
		IdentifierRegistry<Country> countries;

		NodeTools::node_callback_t load_country_party(
			PoliticsManager const& politics_manager, IdentifierRegistry<CountryParty>& country_parties
		) const;

	public:
		CountryManager();

		bool add_country(
			std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
			IdentifierRegistry<CountryParty>&& parties, Country::unit_names_map_t&& unit_names, bool dynamic_tag,
			Country::government_colour_map_t&& alternative_colours
		);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(country, countries)

		bool load_countries(
			GameManager const& game_manager, Dataloader const& dataloader, fs::path const& countries_dir, ast::NodeCPtr root
		);
		bool load_country_data_file(
			GameManager const& game_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root
		);
	};
}
