#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct IdeologyManager;

	struct IdeologyGroup : HasIdentifier {
		friend struct IdeologyManager;

	private:
		IdeologyGroup(std::string_view new_identifier);

	public:
		IdeologyGroup(IdeologyGroup&&) = default;
	};

	struct Ideology : HasIdentifierAndColour {
		friend struct IdeologyManager;

		static constexpr colour_t NO_IDEOLOGY_COLOUR = colour_t::fill_as(colour_t::max_value);

	private:
		IdeologyGroup const& PROPERTY(group);
		const bool PROPERTY_CUSTOM_PREFIX(uncivilised, is);
		const bool PROPERTY(can_reduce_militancy);
		const Date PROPERTY(spawn_date);
		ConditionalWeight PROPERTY(add_political_reform);
		ConditionalWeight PROPERTY(remove_political_reform);
		ConditionalWeight PROPERTY(add_social_reform);
		ConditionalWeight PROPERTY(remove_social_reform);
		ConditionalWeight PROPERTY(add_military_reform);
		ConditionalWeight PROPERTY(add_economic_reform);

		// TODO - willingness to repeal/pass reforms (and its modifiers)

		Ideology(
			std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool new_uncivilised,
			bool new_can_reduce_militancy, Date new_spawn_date, ConditionalWeight&& new_add_political_reform,
			ConditionalWeight&& new_remove_political_reform, ConditionalWeight&& new_add_social_reform,
			ConditionalWeight&& new_remove_social_reform, ConditionalWeight&& new_add_military_reform,
			ConditionalWeight&& new_add_economic_reform
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		Ideology(Ideology&&) = default;
	};

	struct IdeologyManager {
	private:
		IdentifierRegistry<IdeologyGroup> IDENTIFIER_REGISTRY(ideology_group);
		IdentifierRegistry<Ideology> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(ideology, ideologies);

	public:
		bool add_ideology_group(std::string_view identifier);

		bool add_ideology(
			std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised,
			bool can_reduce_militancy, Date spawn_date, ConditionalWeight&& add_political_reform,
			ConditionalWeight&& remove_political_reform, ConditionalWeight&& add_social_reform,
			ConditionalWeight&& remove_social_reform, ConditionalWeight&& add_military_reform,
			ConditionalWeight&& add_economic_reform
		);

		bool load_ideology_file(ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
