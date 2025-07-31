#include "BaseIssue.hpp"

using namespace OpenVic;

BaseIssue::BaseIssue(
	std::string_view new_identifier,
	colour_t new_colour,
	ModifierValue&& new_values,
	BaseIssueGroup const& new_issue_group,
	RuleSet&& new_rules,
	bool new_is_jingoism,
	modifier_type_t new_type
) : Modifier { new_identifier, std::move(new_values), new_type },
	HasColour { new_colour, false },
	issue_group { new_issue_group },
	rules { std::move(new_rules) },
	is_jingoism { new_is_jingoism } {}