#include "PartyIssue.hpp"

using namespace OpenVic;

PartyIssueGroup::PartyIssueGroup(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

PartyIssue::PartyIssue(const std::string_view new_identifier, PartyIssueGroup const& new_group)
	: HasIdentifier { new_identifier }, group { new_group } {}

PartyIssueManager::PartyIssueManager() : party_issue_groups { "party issue groups" }, party_issues { "party issues" } {}

bool PartyIssueManager::add_party_issue_group(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid party issue group identifier - empty!");
		return false;
	}

	return party_issue_groups.add_item({ identifier });
}

bool PartyIssueManager::add_party_issue(const std::string_view identifier, PartyIssueGroup const* group) {
	if (identifier.empty()) {
		Logger::error("Invalid party issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null party issue group for ", identifier);
		return false;
	}

	return party_issues.add_item({ identifier, *group });
}