#pragma once

#include "types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct PartyIssueManager;

	struct PartyIssueGroup : HasIdentifier {
		friend struct PartyIssueManager;

	private:
		PartyIssueGroup(const std::string_view new_identifier);
	
	public:
		PartyIssueGroup(PartyIssueGroup&&) = default;
	};

	struct PartyIssue : HasIdentifier {
		friend struct PartyIssueManager;

	private:
		PartyIssueGroup const& group;

		//TODO - modifiers when party with issue is in power

		PartyIssue(const std::string_view new_identifier, PartyIssueGroup const& new_group);

	public:
		PartyIssue(PartyIssue&&) = default;
	};

	struct PartyIssueManager {
	private:
		IdentifierRegistry<PartyIssueGroup> party_issue_groups;
		IdentifierRegistry<PartyIssue> party_issues;

	public:
		PartyIssueManager();
		
		bool add_party_issue_group(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(PartyIssueGroup, party_issue_group)

		bool add_party_issue(const std::string_view identifier, PartyIssueGroup const* group);
		IDENTIFIER_REGISTRY_ACCESSORS(PartyIssue, party_issue)

		//TODO - loaders
	};
}