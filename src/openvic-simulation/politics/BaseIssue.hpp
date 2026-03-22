#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/politics/RuleSet.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct BaseIssue : Modifier, public HasColour {
	protected:
		BaseIssue(
			std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_values, RuleSet&& new_rules,
			bool new_is_jingoism, modifier_type_t new_type
		);
		BaseIssue(BaseIssue&&) = default;

	public:
		const bool is_jingoism;
		const RuleSet rules;
	};
}
