#include "Country.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

std::string_view CountryParty::get_name() const {
	return name;
}

const Date& CountryParty::get_start_date() const {
	return start_date;
}

const Date& CountryParty::get_end_date() const {
	return end_date;
}

const Ideology& CountryParty::get_ideology() const {
	return ideology;
}

const std::vector<const Issue*>& CountryParty::get_policies() const {
	return policies;
}

CountryParty::CountryParty(
	std::string_view new_name,
	Date new_start_date,
	Date new_end_date,
	const Ideology& new_ideology,
	std::vector<const Issue*>&& new_policies
) : name { new_name },
	start_date { new_start_date },
	end_date { new_end_date },
	ideology { new_ideology },
	policies { std::move(new_policies) } {
}

std::string_view UnitNames::get_identifier() const {
	return identifier;
}

const std::vector<std::string>& UnitNames::get_names() const {
	return names;
}

UnitNames::UnitNames(
	std::string_view new_identifier,
	std::vector<std::string>&& new_names
) : identifier { new_identifier },
	names { std::move(new_names) } {
}

const GraphicalCultureType& Country::get_graphical_culture() const {
	return graphical_culture;
}

const std::vector<CountryParty>& Country::get_parties() const {
	return parties;
}

const std::vector<UnitNames>& Country::get_unit_names() const {
	return unit_names;
}

bool Country::is_dynamic_tag() const {
	return dynamic_tag;
}

Country::Country(
	std::string_view new_identifier,
	colour_t new_color,
	const GraphicalCultureType& new_graphical_culture,
	std::vector<CountryParty>&& new_parties,
	std::vector<UnitNames>&& new_unit_names,
	bool new_dynamic_tag,
	std::map<const GovernmentType*, colour_t>&& new_alternative_colours
) : HasIdentifierAndColour(new_identifier, new_color, false, false),
	graphical_culture { new_graphical_culture },
	parties { std::move(new_parties) },
	unit_names { std::move(new_unit_names) },
	dynamic_tag { new_dynamic_tag },
	alternative_colours { std::move(new_alternative_colours) } {
}

CountryManager::CountryManager()
	: countries { "countries" } {
}

bool CountryManager::add_country(
	std::string_view identifier,
	colour_t color,
	const GraphicalCultureType& graphical_culture,
	std::vector<CountryParty>&& parties,
	std::vector<UnitNames>&& unit_names,
	bool dynamic_tag,
	std::map<const GovernmentType*, colour_t>&& alternative_colours
) {
	if (identifier.empty()) {
		return false;
	}

	return countries.add_item({ identifier, color, graphical_culture, std::move(parties), std::move(unit_names), dynamic_tag, std::move(alternative_colours) });
}

bool CountryManager::load_country_data_file(GameManager& game_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root) {
	colour_t color;
	const GraphicalCultureType* graphical_culture;
	std::vector<CountryParty> country_parties;
	std::vector<UnitNames> unit_names;
	std::map<const GovernmentType*, colour_t> alternative_colours;

	bool ret = expect_dictionary_keys_and_default(
		[&game_manager, &alternative_colours, &name](std::string_view key, ast::NodeCPtr value) -> bool {
			const GovernmentType* colour_gov_type;
			bool ret = game_manager.get_politics_manager().get_government_type_manager().expect_government_type_identifier(assign_variable_callback_pointer(colour_gov_type))(key);

			if (!ret) return false;

			colour_t alternative_colour;
			ret &= expect_colour(assign_variable_callback(alternative_colour))(value);

			if (!ret) return false;

			return alternative_colours.emplace(std::move(colour_gov_type), std::move(alternative_colour)).second;
		},
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(color)),
		"graphical_culture", ONE_EXACTLY, expect_identifier_or_string([&game_manager, &graphical_culture, &name](std::string_view value) -> bool {
			graphical_culture = game_manager.get_pop_manager().get_culture_manager().get_graphical_culture_type_by_identifier(value);
			if (graphical_culture == nullptr) {
				Logger::error("When loading country ", name, ", specified graphical culture ", value, " is invalid!\nCheck that CultureManager has loaded before CountryManager.");
			}

			return graphical_culture != nullptr;
		}),
		"party", ZERO_OR_MORE, [&game_manager, &country_parties, &name](ast::NodeCPtr value) -> bool {
			std::string_view party_name;
			Date start_date, end_date;
			const Ideology* ideology;
			std::vector<const Issue*> policies;

			bool ret = expect_dictionary_keys_and_default(
				[&game_manager, &policies](std::string_view key, ast::NodeCPtr value) -> bool {
					const Issue* policy;
					bool ret = expect_identifier_or_string(
						game_manager.get_politics_manager().get_issue_manager().expect_issue_identifier(
							assign_variable_callback_pointer(policy)
						)
					)(value);

					if (ret && policy->get_group().get_identifier() == key) {
						policies.push_back(policy);
						return true;
					}

					return false;
				},
				"name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(party_name)),
				"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start_date)),
				"end_date", ONE_EXACTLY, expect_date(assign_variable_callback(end_date)),
				"ideology", ONE_EXACTLY, expect_identifier_or_string(game_manager.get_politics_manager().get_ideology_manager().expect_ideology_identifier(assign_variable_callback_pointer(ideology)))
			)(value);

			country_parties.push_back({ party_name, start_date, end_date, *ideology, std::move(policies) });

			return ret; //
		},
		"unit_names", ZERO_OR_ONE, expect_dictionary([&unit_names](std::string_view key, ast::NodeCPtr value) -> bool {
			std::vector<std::string> names;

			bool ret = expect_list(expect_identifier_or_string(
				[&names](std::string_view value) -> bool {
					names.push_back(std::string(value));
					return true;
				}
			))(value);

			unit_names.push_back({ key, std::move(names) });

			return ret;
		})
	)(root);

	ret &= add_country(name, color, *graphical_culture, std::move(country_parties), std::move(unit_names), is_dynamic, std::move(alternative_colours));
	return ret;
}