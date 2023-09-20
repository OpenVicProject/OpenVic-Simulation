#include "Issue.hpp"
#include "types/IdentifierRegistry.hpp"

using namespace OpenVic;

IssueType::IssueType(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

IssueGroup::IssueGroup(const std::string_view new_identifier, IssueType const& new_type, bool ordered) 
	: HasIdentifier { new_identifier }, type { new_type }, ordered { ordered } {}

IssueType const& IssueGroup::get_type() const {
	return type;
}

bool IssueGroup::is_ordered() const {
	return ordered;
}

Issue::Issue(const std::string_view new_identifier, IssueGroup const& new_group, size_t ordinal)
	: HasIdentifier { new_identifier }, group { new_group }, ordinal { ordinal } {}

IssueType const& Issue::get_type() const {
	return group.get_type();
}

IssueGroup const& Issue::get_group() const {
	return group;
}

size_t Issue::get_ordinal() const {
	return ordinal;
}

IssueManager::IssueManager() : issue_types { "issue types" }, issue_groups { "issue groups" }, issues { "issues" } {}

bool IssueManager::add_issue_type(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid issue type identifier - empty!");
		return false;
	}

	return issue_types.add_item({ identifier });
}

bool IssueManager::add_issue_group(const std::string_view identifier, IssueType const* type, bool ordered) {
	if (identifier.empty()) {
		Logger::error("Invalid issue group identifier - empty!");
		return false;
	}

	if (type == nullptr) {
		Logger::error("Null issue type for ", identifier);
		return false;
	}

	return issue_groups.add_item({ identifier, *type, ordered });
}

bool IssueManager::add_issue(const std::string_view identifier, IssueGroup const* group, size_t ordinal) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null issue group for ", identifier);
		return false;
	}

	return issues.add_item({ identifier, *group, ordinal });
}