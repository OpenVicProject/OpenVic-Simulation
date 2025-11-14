#include "CountryParser.hpp"

#include <filesystem>
#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Error.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

using namespace OpenVic;

CountryParser::CountryParser(
	IdentifierRegistry<GovernmentType> const& government_type_registry, //
	IdentifierRegistry<GraphicalCultureType> const& graphical_culture_type_registry, //
	IdentifierPointerRegistry<UnitType> const& unit_type_registry, //
	IdentifierRegistry<PartyPolicyGroup> const& policy_group_registry, //
	IdentifierRegistry<PartyPolicy> const& party_policy_registry, //
	IdentifierRegistry<Ideology> const& ideology_registry, //
	ovdl::v2script::Parser list_parser
)
	: government_type_registry { &government_type_registry }, //
	  graphical_culture_type_registry { &graphical_culture_type_registry }, //
	  unit_type_registry { &unit_type_registry }, //
	  policy_group_registry { &policy_group_registry }, //
	  party_policy_registry { &party_policy_registry }, //
	  ideology_registry { &ideology_registry }, //
	  list_parser { std::move(list_parser) } {}

Error CountryParser::load_country_list() {
	using namespace NodeTools;

	bool is_dynamic = false;

	const bool ret =
		expect_dictionary_reserve_length(tags, [this, &is_dynamic](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "dynamic_tags") {
				return expect_bool([&is_dynamic](bool val) -> bool {
					if (val == is_dynamic) {
						spdlog::warn_s("Redundant \"is_dynamic\", already {}", val ? "true" : "false");
					} else {
						if (is_dynamic) {
							spdlog::warn_s("Changing \"is_dynamic\" back to false");
						}
						is_dynamic = val;
					}
					return true;
				})(value);
			}

			std::string_view path;
			if (expect_string(assign_variable_callback(path))(value)) {
				tags.emplace_back(key, path, is_dynamic);
				return true;
			}

			spdlog::critical_s("Failed to load tag {} in country list.", key);
			return false;
		})(list_parser.get_file_node());

	return ret ? Error::OK : Error::FAILED;
}


Error CountryParser::load_countries_from(
	std::string_view common_folder, Dataloader& dataloader, CountryDefinitionManager& manager
) {
	Error err;

	for (TagData const& data : tags) {
		const fs::path path = dataloader.lookup_file(StringUtils::append_string_views(common_folder, data.path));
		if (Error load_err = load_country(data.tag, data.is_dynamic, Dataloader::parse_defines(path).get_file_node(), manager);
			load_err != Error::OK) {
			err = load_err;
		}
	}
	return err;
}

OV_SPEED_INLINE NodeTools::NodeCallback auto CountryParser::_expect_country_party( //
	IdentifierRegistry<CountryParty>& r_parties_registry
) {
	using namespace NodeTools;

	return [this, &r_parties_registry](ast::NodeCPtr value) -> bool {
		std::string_view party_name;
		Date start_date, end_date;
		Ideology const* ideology = nullptr;
		IndexedFlatMap<PartyPolicyGroup, PartyPolicy const*> policies { policy_group_registry->get_items() };

		bool ret = expect_dictionary_keys_and_default(
			[&policies, &party_name, this](std::string_view key, ast::NodeCPtr value) -> bool {
				return policy_group_registry->expect_item_str(
					[this, &policies, value, &party_name](PartyPolicyGroup const& party_policy_group) -> bool {
						PartyPolicy const*& policy = policies.at(party_policy_group);

						if (policy != nullptr) {
							spdlog::error_s(
								"Country party \"{}\" has duplicate entry for party policy group \"{}\"",
								party_name, party_policy_group
							);
							return false;
						}

						return party_policy_registry->expect_item_identifier(
							[&policy, &party_policy_group](PartyPolicy const& party_policy) -> bool {
								if (&party_policy.get_issue_group() == &party_policy_group) {
									policy = &party_policy;
									return true;
								}

								// TODO - change this back to error/false once TGC no longer has this issue
								spdlog::warn_s(
									"Invalid party policy \"{}\", group is \"{}\" when \"{}\" was expected.",
									party_policy,
									party_policy.get_issue_group(),
									party_policy_group
								);
								return true;
							}, false
						)(value);
					}, false, false
				)(key);
			},
			"name", ONE_EXACTLY, expect_string(assign_variable_callback(party_name)),
			"start_date", ONE_EXACTLY, expect_date(assign_variable_callback(start_date)),
			"end_date", ONE_EXACTLY, expect_date(assign_variable_callback(end_date)),
			"ideology", ONE_EXACTLY,
				ideology_registry->expect_item_identifier(assign_variable_callback_pointer(ideology), false)
		)(value);

		if (ideology == nullptr) {
			spdlog::warn_s("Country party {} has no ideology, defaulting to nullptr / no ideology", party_name);
		}

		ret &= r_parties_registry.emplace_item(
			party_name, duplicate_warning_callback, party_name, start_date, end_date, ideology, std::move(policies)
		);

		return ret;
	};
}

Error CountryParser::load_country(
	std::string_view tag, bool is_dynamic_tag, ast::NodeCPtr country_root, CountryDefinitionManager& definition_manager
) {
	using namespace NodeTools;

	colour_t colour;
	GraphicalCultureType const* graphical_culture;
	IdentifierRegistry<CountryParty> parties { "country parties" };
	CountryDefinition::unit_names_map_t unit_names;
	CountryDefinition::government_colour_map_t alternative_colours;

	bool ret = expect_dictionary_keys_and_default(
		[this, &alternative_colours](std::string_view key, ast::NodeCPtr value) -> bool {
			return government_type_registry->expect_item_str(
				[&alternative_colours, value](GovernmentType const& government_type) -> bool {
					return expect_colour(map_callback(alternative_colours, &government_type))(value);
				}, false, false
			)(key);
		},
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
		"graphical_culture", ONE_EXACTLY,
			graphical_culture_type_registry->expect_item_identifier(
				assign_variable_callback_pointer(graphical_culture)
			),
		"party", ZERO_OR_MORE, _expect_country_party(parties),
		"unit_names", ZERO_OR_ONE,
			unit_type_registry->expect_item_dictionary_reserve_length(
				unit_names,
				[&unit_names](UnitType const& unit, ast::NodeCPtr value) -> bool {
					return name_list_callback(map_callback(unit_names, &unit))(value);
				}
			)
	)(country_root);

	ret &= definition_manager.add_country(
		tag, colour, graphical_culture, std::move(parties), std::move(unit_names), is_dynamic_tag,
		std::move(alternative_colours)
	);

	return ret ? Error::OK : Error::FAILED;
}

NodeTools::node_callback_t CountryParser::expect_country_party(IdentifierRegistry<CountryParty>& r_parties_registry) {
	return _expect_country_party(r_parties_registry);
}

Error CountryParser::load_country_colours(ast::NodeCPtr colours_root, CountryDefinitionManager& manager) {
	using namespace NodeTools;

	const bool ret = manager.expect_country_definition_dictionary_and_default(
		[](std::string_view key, ast::NodeCPtr value) -> bool {
			spdlog::warn_s("country_colors references country tag {} which is not defined!", key);
			return true;
		},
		[](CountryDefinition& country, ast::NodeCPtr colour_node) -> bool {
			colour_t primary, secondary, tertiary;

			const bool ret = expect_dictionary_keys(
				"color1", ONE_EXACTLY, expect_colour(assign_variable_callback(primary)),
				"color2", ONE_EXACTLY, expect_colour(assign_variable_callback(secondary)),
				"color3", ONE_EXACTLY, expect_colour(assign_variable_callback(tertiary))
			)(colour_node);

			country.set_primary_unit_colour(primary);
			country.set_secondary_unit_colour(secondary);
			country.set_tertiary_unit_colour(tertiary);

			return ret;
		}
	)(colours_root);

	return ret ? Error::OK : Error::FAILED;
}
