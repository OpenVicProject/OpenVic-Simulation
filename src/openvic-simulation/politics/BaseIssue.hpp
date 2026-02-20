#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/politics/RuleSet.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct BaseIssueGroup;

	struct BaseIssue : Modifier, public HasColour {
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
	public:
		const bool is_jingoism;
		const RuleSet rules;

		BaseIssueGroup const& issue_group;		
	};

	struct BaseIssueGroup : HasIdentifier {
		friend struct IssueManager;

	private:
		memory::vector<BaseIssue const*> SPAN_PROPERTY(issues);

	protected:
		BaseIssueGroup(std::string_view new_identifier): HasIdentifier { new_identifier } {}
		BaseIssueGroup(BaseIssueGroup&&) = default;
	};
}