#pragma once

#include "openvic-simulation/misc/Decision.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct DecisionManager {
	private:
		IdentifierRegistry<Decision> IDENTIFIER_REGISTRY(decision);

	public:
		bool add_decision(
			std::string_view identifier, bool alert, bool news, std::string_view news_title, std::string_view news_desc_long,
			std::string_view news_desc_medium, std::string_view news_desc_short, std::string_view picture,
			ConditionScript&& potential, ConditionScript&& allow, ConditionalWeightFactorMul&& ai_will_do,
			EffectScript&& effect
		);

		bool load_decision_file(ovdl::v2script::ast::Node const* root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
