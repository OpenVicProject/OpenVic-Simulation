#pragma once

#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct IdeologyManager;

	struct IdeologyGroup : HasIdentifier {
	public:
		IdeologyGroup(std::string_view new_identifier);
		IdeologyGroup(IdeologyGroup&&) = default;
	};

	struct Ideology : HasIdentifierAndColour, HasIndex<Ideology, ideology_index_t> {
		friend struct IdeologyManager;

		static constexpr colour_t NO_IDEOLOGY_COLOUR = colour_t::fill_as(colour_t::max_value);

	private:
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
		const bool is_allowed_for_uncivilised;
		const std::optional<Date> spawn_date;

		IdeologyGroup const& group;

		Ideology(
			std::string_view new_identifier,
			index_t new_index,
			colour_t new_colour,
			IdeologyGroup const& new_group,
			bool new_is_allowed_for_uncivilised,
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
}
