#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct IdeologyManager;

	struct IdeologyGroup : HasIdentifier {
	public:
		IdeologyGroup(std::string_view new_identifier);
		IdeologyGroup(IdeologyGroup&&) = default;
	};

	struct Ideology : HasIdentifierAndColour, HasIndex<Ideology> {
		friend struct IdeologyManager;

		static constexpr colour_t NO_IDEOLOGY_COLOUR = colour_t::fill_as(colour_t::max_value);

	private:
		const bool PROPERTY_CUSTOM_PREFIX(uncivilised, is);
		const std::optional<Date> PROPERTY(spawn_date);
		ConditionalWeightBase PROPERTY(add_political_reform);
		ConditionalWeightBase PROPERTY(remove_political_reform);
		ConditionalWeightBase PROPERTY(add_social_reform);
		ConditionalWeightBase PROPERTY(remove_social_reform);
		ConditionalWeightBase PROPERTY(add_military_reform);
		ConditionalWeightBase PROPERTY(add_economic_reform);

		// TODO - willingness to repeal/pass reforms (and its modifiers)

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const bool can_reduce_consciousness;
		const bool can_reduce_militancy;

		IdeologyGroup const& group;

		Ideology(
			std::string_view new_identifier,
			index_t new_index,
			colour_t new_colour,
			IdeologyGroup const& new_group,
			bool new_uncivilised,
			bool new_can_reduce_consciousness,
			bool new_can_reduce_militancy,
			std::optional<Date> new_spawn_date,
			ConditionalWeightBase&& new_add_political_reform,
			ConditionalWeightBase&& new_remove_political_reform,
			ConditionalWeightBase&& new_add_social_reform,
			ConditionalWeightBase&& new_remove_social_reform,
			ConditionalWeightBase&& new_add_military_reform,
			ConditionalWeightBase&& new_add_economic_reform
		);
		Ideology(Ideology&&) = default;
	};

	struct IdeologyManager {
	private:
		IdentifierRegistry<IdeologyGroup> IDENTIFIER_REGISTRY(ideology_group);
		IdentifierRegistry<Ideology> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(ideology, ideologies);

	public:
		bool add_ideology_group(std::string_view identifier);

		bool add_ideology(
			std::string_view identifier,
			colour_t colour,
			IdeologyGroup const* group,
			bool uncivilised,
			bool can_reduce_consciousness,
			bool can_reduce_militancy,
			std::optional<Date> spawn_date,
			ConditionalWeightBase&& add_political_reform,
			ConditionalWeightBase&& remove_political_reform,
			ConditionalWeightBase&& add_social_reform,
			ConditionalWeightBase&& remove_social_reform,
			ConditionalWeightBase&& add_military_reform,
			ConditionalWeightBase&& add_economic_reform
		);

		bool load_ideology_file(ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
