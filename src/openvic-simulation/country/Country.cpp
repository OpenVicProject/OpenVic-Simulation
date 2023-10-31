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

CountryParty::CountryParty(
	std::string_view new_identifier, Date new_start_date, Date new_end_date, Ideology const& new_ideology,
	policy_map_t&& new_policies
) : HasIdentifier { new_identifier }, start_date { new_start_date }, end_date { new_end_date }, ideology { new_ideology },
	policies { std::move(new_policies) } {}

Date CountryParty::get_start_date() const {
	return start_date;
}

Date CountryParty::get_end_date() const {
	return end_date;
}

Ideology const& CountryParty::get_ideology() const {
	return ideology;
}

CountryParty::policy_map_t const& CountryParty::get_policies() const {
	return policies;
}

Country::Country(
	std::string_view new_identifier, colour_t new_colour, GraphicalCultureType const& new_graphical_culture,
	IdentifierRegistry<CountryParty>&& new_parties, unit_names_map_t&& new_unit_names, bool new_dynamic_tag,
	government_colour_map_t&& new_alternative_colours
) : HasIdentifierAndColour { new_identifier, new_colour, false, false }, graphical_culture { new_graphical_culture },
	parties { std::move(new_parties) }, unit_names { std::move(new_unit_names) }, dynamic_tag { new_dynamic_tag },
	alternative_colours { std::move(new_alternative_colours) } {}

GraphicalCultureType const& Country::get_graphical_culture() const {
	return graphical_culture;
}

Country::unit_names_map_t const& Country::get_unit_names() const {
	return unit_names;
}

bool Country::is_dynamic_tag() const {
	return dynamic_tag;
}

Country::government_colour_map_t const& Country::get_alternative_colours() const {
	return alternative_colours;
}

CountryManager::CountryManager() : countries { "countries" } {}

bool CountryManager::add_country(
	std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
	IdentifierRegistry<CountryParty>&& parties, Country::unit_names_map_t&& unit_names, bool dynamic_tag,
	Country::government_colour_map_t&& alternative_colours
) {
	if (identifier.empty()) {
		Logger::error("Invalid country identifier - empty!");
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid country colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	if (graphical_culture == nullptr) {
		Logger::error("Null graphical culture for country ", identifier);
		return false;
	}

	return countries.add_item({
		identifier, colour, *graphical_culture, std::move(parties), std::move(unit_names), dynamic_tag,
		std::move(alternative_colours)
	});
}

bool CountryManager::load_countries(
	GameManager const& game_manager, Dataloader const& dataloader, fs::path const& countries_dir, ast::NodeCPtr root
) {
	bool is_dynamic = false;

	const bool ret = expect_dictionary_reserve_length(
		countries,
		[this, &game_manager, &is_dynamic, &dataloader, &countries_dir](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "dynamic_tags") {
				return expect_bool([&is_dynamic](bool val) -> bool {
					if (val == is_dynamic) {
						Logger::warning("Redundant \"is_dynamic\", already ", val ? "true" : "false");
					} else {
						if (is_dynamic) {
							Logger::warning("Changing \"is_dynamic\" back to false");
						}
						is_dynamic = val;
					}
					return true;
				})(value);
			}
			if (expect_string(
				[this, &game_manager, is_dynamic, &dataloader, &countries_dir, &key](std::string_view filepath) -> bool {
					if (load_country_data_file(
						game_manager, key, is_dynamic,
						Dataloader::parse_defines(dataloader.lookup_file_case_insensitive(countries_dir / filepath)).get_file_node()
					)) {
						return true;
					}
					Logger::error("Failed to load country data file: ", filepath);
					return false;
				}
			)(value)) {
				return true;
			}
			Logger::error("Failed to load country: ", key);
			return false;
		}
	)(root);
	lock_countries();
	return ret;
}

node_callback_t CountryManager::load_country_party(
	PoliticsManager const& politics_manager, IdentifierRegistry<CountryParty>& country_parties
) const {
	return [&politics_manager, &country_parties](ast::NodeCPtr value) -> bool {
		std::string_view party_name;
		Date start_date, end_date;
		Ideology const* ideology;
		CountryParty::policy_map_t policies;

		bool ret = expect_dictionary_keys_and_default(
			[&politics_manager, &policies, &party_name](std::string_view key, ast::NodeCPtr value) -> bool {
				return politics_manager.get_issue_manager().expect_issue_group_str(
					[&politics_manager, &policies, value, &party_name](IssueGroup const& group) -> bool {
						if (policies.contains(&group)) {
							Logger::error("Country party ", party_name, " has duplicate entry for ", group.get_identifier());
							return false;
						}
						return politics_manager.get_issue_manager().expect_issue_identifier(
							[&policies, &group](Issue const& issue) -> bool {
								if (&issue.get_group() == &group) {
									policies.emplace(&group, &issue);
									return true;
								}
								// TODO - change this back to error/false once TGC no longer has this issue
								Logger::warning("Invalid policy ", issue.get_identifier(), ", group is ",
									issue.get_group().get_identifier(), " when ", group.get_identifier(), " was expected");
								return true;
							}
						)(value);
					}
				)(key);
			},
			"name", ONE_EXACTLY, expect_string(assign_variable_callback(party_name)),
			"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start_date)),
			"end_date", ONE_EXACTLY, expect_date(assign_variable_callback(end_date)),
			"ideology", ONE_EXACTLY,
				politics_manager.get_ideology_manager().expect_ideology_identifier(assign_variable_callback_pointer(ideology))
		)(value);

		ret &= country_parties.add_item(
			{ party_name, start_date, end_date, *ideology, std::move(policies) }, duplicate_warning_callback
		);

		return ret;
	};
}

bool CountryManager::load_country_data_file(
	GameManager const& game_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root
) {
	colour_t colour;
	GraphicalCultureType const* graphical_culture;
	IdentifierRegistry<CountryParty> parties { "country parties" };
	Country::unit_names_map_t unit_names;
	Country::government_colour_map_t alternative_colours;
	bool ret = expect_dictionary_keys_and_default(
		[&game_manager, &alternative_colours, &name](std::string_view key, ast::NodeCPtr value) -> bool {
			return game_manager.get_politics_manager().get_government_type_manager().expect_government_type_str(
				[&alternative_colours, &name, &value](GovernmentType const& government_type) -> bool {
					if (alternative_colours.contains(&government_type)) {
						Logger::error(
							"Country ", name, " has duplicate entry for ", government_type.get_identifier(),
							" alternative colour"
						);
						return false;
					}
					return expect_colour([&alternative_colours, &government_type](colour_t colour) -> bool {
						return alternative_colours.emplace(&government_type, std::move(colour)).second;
					})(value);
				}
			)(key);
		},
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"graphical_culture", ONE_EXACTLY,
			game_manager.get_pop_manager().get_culture_manager().expect_graphical_culture_type_identifier(
				assign_variable_callback_pointer(graphical_culture)
			),
		"party", ZERO_OR_MORE, load_country_party(game_manager.get_politics_manager(), parties),
		"unit_names", ZERO_OR_ONE,
			game_manager.get_military_manager().get_unit_manager().expect_unit_dictionary(
				[&unit_names, &name](Unit const& unit, ast::NodeCPtr value) -> bool {
					if (unit_names.contains(&unit)) {
						Logger::error("Country ", name, " has duplicate entry for ", unit.get_identifier(), " name list");
						return false;
					}
					return name_list_callback([&unit_names, &unit](std::vector<std::string>&& list) -> bool {
						return unit_names.emplace(&unit, std::move(list)).second;
					})(value);
				}
			)
	)(root);

	ret &= add_country(
		name, colour, graphical_culture, std::move(parties), std::move(unit_names), is_dynamic,
		std::move(alternative_colours)
	);
	return ret;
}
