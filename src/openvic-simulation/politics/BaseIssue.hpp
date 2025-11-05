#pragma once

#include <string_view>

#include "openvic-simulation/core/container/HasColour.hpp"
#include "openvic-simulation/core/container/HasIdentifier.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/politics/Rule.hpp"

namespace OpenVic {
	struct BaseIssueGroup;

	struct BaseIssue : Modifier, HasColour {
	private:
		BaseIssueGroup const& PROPERTY(issue_group);
		RuleSet PROPERTY(rules);
		const bool PROPERTY(is_jingoism);

	protected:
		BaseIssue(
			std::string_view new_identifier,
			colour_t new_colour,
			ModifierValue&& new_values,
			BaseIssueGroup const& new_issue_group,
			RuleSet&& new_rules,
			bool new_is_jingoism,
			modifier_type_t new_type
		);
		BaseIssue(BaseIssue&&) = default;
	};

	struct BaseIssueGroup : HasIdentifier {
		friend struct IssueManager;

	private:
		memory::vector<BaseIssue const*> PROPERTY(issues);

	protected:
		BaseIssueGroup(std::string_view new_identifier): HasIdentifier { new_identifier } {}
		BaseIssueGroup(BaseIssueGroup&&) = default;
	};
}