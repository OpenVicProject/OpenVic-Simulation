#pragma once

#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
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

		bool load_ideology_file(ovdl::v2script::ast::Node const* root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
