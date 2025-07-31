#include "CountryDefinition.hpp"

#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryDefinition::CountryDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	GraphicalCultureType const& new_graphical_culture,
	IdentifierRegistry<CountryParty>&& new_parties,
	unit_names_map_t&& new_unit_names,
	bool new_dynamic_tag,
	government_colour_map_t&& new_alternative_colours,
	colour_t new_primary_unit_colour,
	colour_t new_secondary_unit_colour,
	colour_t new_tertiary_unit_colour
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	graphical_culture { new_graphical_culture },
	parties { std::move(new_parties) },
	unit_names { std::move(new_unit_names) },
	dynamic_tag { new_dynamic_tag },
	alternative_colours { std::move(new_alternative_colours) },
	primary_unit_colour { new_primary_unit_colour },
	secondary_unit_colour { new_secondary_unit_colour },
	tertiary_unit_colour { new_tertiary_unit_colour } {}

bool CountryDefinitionManager::add_country(
	std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
	IdentifierRegistry<CountryParty>&& parties, CountryDefinition::unit_names_map_t&& unit_names, bool dynamic_tag,
	CountryDefinition::government_colour_map_t&& alternative_colours
) {
	if (identifier.empty()) {
		Logger::error("Invalid country identifier - empty!");
		return false;
	}
	if (!valid_basic_identifier(identifier)) {
		Logger::error(
			"Invalid country identifier: ", identifier, " (can only contain alphanumeric characters and underscores)"
		);
		return false;
	}
	if (graphical_culture == nullptr) {
		Logger::error("Null graphical culture for country ", identifier);
		return false;
	}

	static constexpr colour_t default_colour = colour_t::fill_as(colour_t::max_value);

	return country_definitions.emplace_item(
		identifier,
		identifier, colour, get_country_definition_count(), *graphical_culture, std::move(parties), std::move(unit_names),
		dynamic_tag, std::move(alternative_colours),
		/* Default to country colour for the chest and grey for the others. Update later if necessary. */
		colour, default_colour, default_colour
	);
}

bool CountryDefinitionManager::load_countries(
	DefinitionManager const& definition_manager, Dataloader const& dataloader, ast::NodeCPtr root
) {
	static constexpr std::string_view common_dir = "common/";
	bool is_dynamic = false;

	const bool ret = expect_dictionary_reserve_length(
		country_definitions,
		[this, &definition_manager, &is_dynamic, &dataloader](std::string_view key, ast::NodeCPtr value) -> bool {
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
				[this, &definition_manager, is_dynamic, &dataloader, &key](std::string_view filepath) -> bool {
					if (load_country_data_file(
						definition_manager, key, is_dynamic,
						Dataloader::parse_defines(
							dataloader.lookup_file(StringUtils::append_string_views(common_dir, filepath))
						).get_file_node()
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
	lock_country_definitions();
	return ret;
}

bool CountryDefinitionManager::load_country_colours(ast::NodeCPtr root) {
	return country_definitions.expect_item_dictionary_and_default(
		[](std::string_view key, ast::NodeCPtr value) -> bool {
			Logger::warning("country_colors.txt references country tag ", key, " which is not defined!");
			return true;
		},
		[](CountryDefinition& country, ast::NodeCPtr colour_node) -> bool {
			return expect_dictionary_keys(
				"color1", ONE_EXACTLY, expect_colour(assign_variable_callback(country.primary_unit_colour)),
				"color2", ONE_EXACTLY, expect_colour(assign_variable_callback(country.secondary_unit_colour)),
				"color3", ONE_EXACTLY, expect_colour(assign_variable_callback(country.tertiary_unit_colour))
			)(colour_node);
		}
	)(root);
}

node_callback_t CountryDefinitionManager::load_country_party(
	PoliticsManager const& politics_manager, IdentifierRegistry<CountryParty>& country_parties
) const {
	return [&politics_manager, &country_parties](ast::NodeCPtr value) -> bool {
		std::string_view party_name;
		Date start_date, end_date;
		Ideology const* ideology = nullptr;
		IndexedMap<PartyPolicyGroup, PartyPolicy const*> policies { politics_manager.get_issue_manager().get_party_policy_groups() };

		bool ret = expect_dictionary_keys_and_default(
			[&politics_manager, &policies, &party_name](std::string_view key, ast::NodeCPtr value) -> bool {
				return politics_manager.get_issue_manager().expect_party_policy_group_str(
					[&politics_manager, &policies, value, &party_name](PartyPolicyGroup const& party_policy_group) -> bool {
						PartyPolicy const* policy = policies[party_policy_group];

						if (policy != nullptr) {
							Logger::error(
								"Country party \"", party_name, "\" has duplicate entry for party policy group \"",
								party_policy_group.get_identifier(), "\""
							);
							return false;
						}

						return politics_manager.get_issue_manager().expect_party_policy_identifier(
							[&party_policy_group, &policy](PartyPolicy const& party_policy) -> bool {
								if (&party_policy.get_issue_group() == &party_policy_group) {
									policy = &party_policy;
									return true;
								}

								// TODO - change this back to error/false once TGC no longer has this issue
								Logger::warning(
									"Invalid party policy \"", party_policy.get_identifier(), "\", group is \"",
									party_policy.get_issue_group().get_identifier(), "\" when \"", party_policy_group.get_identifier(),
									"\" was expected."
								);
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

		if (ideology == nullptr) {
			Logger::warning("Country party ", party_name, " has no ideology, defaulting to nullptr / no ideology");
		}

		ret &= country_parties.emplace_item(
			party_name,
			duplicate_warning_callback,
			party_name, start_date, end_date, ideology, std::move(policies)
		);

		return ret;
	};
}

bool CountryDefinitionManager::load_country_data_file(
	DefinitionManager const& definition_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root
) {
	colour_t colour;
	GraphicalCultureType const* graphical_culture;
	IdentifierRegistry<CountryParty> parties { "country parties" };
	CountryDefinition::unit_names_map_t unit_names;
	CountryDefinition::government_colour_map_t alternative_colours;
	bool ret = expect_dictionary_keys_and_default(
		[&definition_manager, &alternative_colours](std::string_view key, ast::NodeCPtr value) -> bool {
			return definition_manager.get_politics_manager().get_government_type_manager().expect_government_type_str(
				[&alternative_colours, value](GovernmentType const& government_type) -> bool {
					return expect_colour(map_callback(alternative_colours, &government_type))(value);
				}
			)(key);
		},
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"graphical_culture", ONE_EXACTLY,
			definition_manager.get_pop_manager().get_culture_manager().expect_graphical_culture_type_identifier(
				assign_variable_callback_pointer(graphical_culture)
			),
		"party", ZERO_OR_MORE, load_country_party(definition_manager.get_politics_manager(), parties),
		"unit_names", ZERO_OR_ONE,
			definition_manager.get_military_manager().get_unit_type_manager().expect_unit_type_dictionary_reserve_length(
				unit_names,
				[&unit_names](UnitType const& unit, ast::NodeCPtr value) -> bool {
					return name_list_callback(map_callback(unit_names, &unit))(value);
				}
			)
	)(root);

	ret &= add_country(
		name, colour, graphical_culture, std::move(parties), std::move(unit_names), is_dynamic, std::move(alternative_colours)
	);
	return ret;
}
