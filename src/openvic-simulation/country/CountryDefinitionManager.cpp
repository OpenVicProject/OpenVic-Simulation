#include "CountryDefinitionManager.hpp"

#include <string_view>
#include <type_traits>

#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/registries/OwningRegistry_Modify.hpp"
#include "openvic-simulation/types/registries/OwningRegistry_NodeTools.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool CountryDefinitionManager::add_country(
	std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
	std::remove_const_t<decltype(CountryDefinition::parties)>&& parties,
	decltype(CountryDefinition::ship_names)&& ship_names,
	bool is_dynamic_tag,
	decltype(CountryDefinition::alternative_colours)&& alternative_colours
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid country identifier - empty!");
		return false;
	}
	if (!valid_basic_identifier(identifier)) {
		spdlog::error_s(
			"Invalid country identifier: {} (can only contain alphanumeric characters and underscores)", identifier
		);
		return false;
	}
	if (graphical_culture == nullptr) {
		spdlog::error_s("Null graphical culture for country {}", identifier);
		return false;
	}

	static constexpr colour_t default_colour = colour_t::fill_as(colour_t::max_value);

	return try_emplace_item(
		country_definitions,
		identifier,
		identifier, colour, country_definitions.size(), *graphical_culture,
		std::move(parties), std::move(ship_names), is_dynamic_tag, std::move(alternative_colours),
		/* Default to country colour for the chest and grey for the others. Update later if necessary. */
		colour, default_colour, default_colour
	);
}

bool CountryDefinitionManager::load_countries(
	DefinitionManager const& definition_manager,
	Dataloader const& dataloader,
	ovdl::v2script::ast::Node const* root
) {
	static constexpr std::string_view common_dir = "common/";
	bool is_dynamic = false;

	const bool ret = expect_dictionary_and_length(
		[this](const std::size_t country_count) -> std::size_t {
			country_definitions.clear_and_reserve(country_index_t(country_count));
			return country_count;
		},
		[this, &definition_manager, &is_dynamic, &dataloader](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
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
			if (expect_string(
				[this, &definition_manager, is_dynamic, &dataloader, &key](std::string_view filepath) -> bool {
					if (load_country_data_file(
						definition_manager, key, is_dynamic,
						Dataloader::parse_defines(
							dataloader.lookup_file(append_string_views(common_dir, filepath))
						).get_file_node()
					)) {
						return true;
					}
					spdlog::critical_s("Failed to load country data file: {}", filepath);
					return false;
				}
			)(value)) {
				return true;
			}
			spdlog::critical_s("Failed to load country: {}", key);
			return false;
		}
	)(root);
	lock_registry(country_definitions);
	return ret;
}

bool CountryDefinitionManager::load_country_colours(ovdl::v2script::ast::Node const* root) {
	return NodeTools::expect_list_and_length(
		NodeTools::default_length_callback,
		NodeTools::expect_assign(
			[this](std::string_view identifier, ovdl::v2script::ast::Node const* node) -> bool {
				const auto it = find(country_definitions, identifier);
				if (it == country_definitions.end()) {
					spdlog::warn_s("country_colors.txt references country tag {} which is not defined!", identifier);
					return true;
				}
				
				CountryDefinition& country = *it;
				return expect_dictionary_keys(
					"color1", ONE_EXACTLY, expect_colour(assign_variable_callback(country.primary_unit_colour)),
					"color2", ONE_EXACTLY, expect_colour(assign_variable_callback(country.secondary_unit_colour)),
					"color3", ONE_EXACTLY, expect_colour(assign_variable_callback(country.tertiary_unit_colour))
				)(node);
			}
		)
	)(root);
}

struct CountryPartyDto { // data transfer object
	std::string_view identifier;
	Date start_date;
	Date end_date;
	Ideology const* ideology;
	memory::FixedVector<PartyPolicy const*, party_policy_group_index_t> policies;
};

static NodeTools::node_callback_t load_country_party(
	PoliticsManager const& politics_manager,
	memory::vector<CountryPartyDto>& country_parties
) {
	return [&politics_manager, &country_parties](ovdl::v2script::ast::Node const* value) -> bool {
		std::string_view party_name;
		Date start_date, end_date;
		Ideology const* ideology = nullptr;
		memory::FixedVector<PartyPolicy const*, party_policy_group_index_t> policies {
			party_policy_group_index_t(politics_manager.get_issue_manager().get_party_policy_group_count()),
			nullptr
		};

		bool ret = expect_dictionary_keys_and_default(
			[&politics_manager, &policies, &party_name](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
				return politics_manager.get_issue_manager().expect_party_policy_group_str(
					[&politics_manager, &policies, value, &party_name](PartyPolicyGroup const& party_policy_group) -> bool {
						PartyPolicy const*& policy = policies[party_policy_group.index];

						if (policy != nullptr) {
							spdlog::error_s(
								"Country party \"{}\" has duplicate entry for party policy group \"{}\"",
								party_name, party_policy_group
							);
							return false;
						}

						return politics_manager.get_issue_manager().expect_party_policy_identifier(
							[&party_policy_group, &policy](PartyPolicy const& party_policy) -> bool {
								if (&party_policy.group == &party_policy_group) {
									policy = &party_policy;
									return true;
								}

								// TODO - change this back to error/false once TGC no longer has this issue
								spdlog::warn_s(
									"Invalid party policy \"{}\", group is \"{}\" when \"{}\" was expected.",
									party_policy,
									party_policy.group,
									party_policy_group
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
			spdlog::warn_s("Country party {} has no ideology, defaulting to nullptr / no ideology", party_name);
		}

		if (ret) {
			country_parties.emplace_back(
				party_name,
				start_date,
				end_date,
				ideology,
				std::move(policies)
			);
		}

		return ret;
	};
}

bool CountryDefinitionManager::load_country_data_file(
	DefinitionManager const& definition_manager,
	std::string_view name,
	bool is_dynamic_tag,
	ovdl::v2script::ast::Node const* root
) {
	colour_t colour;
	GraphicalCultureType const* graphical_culture;
	auto const& ship_types = definition_manager.get_military_manager().get_unit_type_manager().get_ship_types();
	decltype(CountryDefinition::ship_names) ship_names {
		ship_types.size(),
		[](const ship_type_index_t) { return create_empty; }
	};
	decltype(CountryDefinition::alternative_colours) alternative_colours;
	memory::vector<CountryPartyDto> party_dtos;
	bool ret = expect_dictionary_keys_and_default(
		[&definition_manager, &alternative_colours](std::string_view key, ovdl::v2script::ast::Node const* value) -> bool {
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
		"party", ZERO_OR_MORE, load_country_party(definition_manager.get_politics_manager(), party_dtos),
		"unit_names", ZERO_OR_ONE, expect_item_dictionary(
				ship_types,
				[&ship_names](const ship_type_index_t ship_type_index, ovdl::v2script::ast::Node const* value) -> bool {
					return name_list_callback(
						[&ship_names, ship_type_index](memory::FixedVector<memory::string>&& names) -> bool {
							ship_names[ship_type_index] = std::move(names);
							return true;
						}
					)(value);
				}
			)
	)(root);

	OwningRegistry<CountryParty, std::size_t> parties { create_empty, party_dtos.size() };

	for (CountryPartyDto& dto : party_dtos) {
		ret &= try_emplace_item(
			parties,
			dto.identifier,
			duplicate_warning_cb,
			dto.identifier, dto.start_date, dto.end_date, dto.ideology, std::move(dto.policies)
		);
	}
	lock_registry(parties);

	ret &= add_country(
		name, colour, graphical_culture,
		std::move(parties), std::move(ship_names),
		is_dynamic_tag, std::move(alternative_colours)
	);
	return ret;
}