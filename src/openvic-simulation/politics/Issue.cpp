#include "Issue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

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

bool IssueManager::_load_issue_group(size_t& expected_issues, const std::string_view identifier, IssueType const* type, ast::NodeCPtr node) {
	bool ordered = false;
	return expect_dictionary_keys_and_length(
		[&expected_issues](size_t size) -> size_t {
			expected_issues += size;
			return size;
		}, ALLOW_OTHER_KEYS,
		"next_step_only", ONE_EXACTLY, [&expected_issues, &ordered](ast::NodeCPtr node) -> bool {
			expected_issues--;
			return expect_bool(assign_variable_callback(ordered))(node);
		}
	)(node) && add_issue_group(identifier, type, ordered);
}

bool IssueManager::_load_issue(size_t& ordinal, const std::string_view identifier, IssueGroup const* group, ast::NodeCPtr node) {
	//TODO: conditions to allow, policy modifiers, policy rule changes
	return add_issue(identifier, group, ordinal);
}

/* REQUIREMENTS:
 * POL-18, POL-19, POL-21, POL-22, POL-23, POL-24, POL-26, POL-27, POL-28, POL-29, POL-31, POL-32, POL-33,
 * POL-35, POL-36, POL-37, POL-38, POL-40, POL-41, POL-43, POL-44, POL-45, POL-46, POL-48, POL-49, POL-50,
 * POL-51, POL-52, POL-53, POL-55, POL-56, POL-57, POL-59, POL-60, POL-62, POL-63, POL-64, POL-66, POL-67,
 * POL-68, POL-69, POL-71, POL-72, POL-73, POL-74, POL-75, POL-77, POL-78, POL-79, POL-80, POL-81, POL-83,
 * POL-84, POL-85, POL-86, POL-87, POL-89, POL-90, POL-91, POL-92, POL-93, POL-95, POL-96, POL-97, POL-98,
 * POL-99, POL-101, POL-102, POL-103, POL-104, POL-105, POL-107, POL-108, POL-109, POL-110, POL-111, POL-113,
 * POL-113, POL-114, POL-115, POL-116
*/
bool IssueManager::load_issues_file(ast::NodeCPtr root) {
	size_t expected_issue_groups = 0;
	bool ret = expect_dictionary_reserve_length(issue_types, 
		[this, &expected_issue_groups](std::string_view key, ast::NodeCPtr value) -> bool {
			return expect_list_and_length(
				[&expected_issue_groups](size_t size) -> size_t {
					expected_issue_groups += size;
					return 0;
				}, success_callback
			)(value) && add_issue_type(key);
		}
	)(root);
	lock_issue_types();

	size_t expected_issues = 0;
	issue_groups.reserve(issue_groups.size() + expected_issue_groups);
	ret &= expect_dictionary_reserve_length(issue_groups,
		[this, &expected_issues](std::string_view type_key, ast::NodeCPtr type_value) -> bool {
			IssueType const* issue_type = get_issue_type_by_identifier(type_key);
			return expect_dictionary(
				[this, issue_type, &expected_issues](std::string_view key, ast::NodeCPtr value) -> bool {
					return _load_issue_group(expected_issues, key, issue_type, value);
				}
			)(type_value);
		}
	)(root);
	lock_issue_groups();

	issues.reserve(issues.size() + expected_issues);
	ret &= expect_dictionary([this](std::string_view type_key, ast::NodeCPtr type_value) -> bool {
		return expect_dictionary([this](std::string_view group_key, ast::NodeCPtr group_value) -> bool {
			IssueGroup const* issue_group = get_issue_group_by_identifier(group_key);
			size_t ordinal = 0;
			return expect_dictionary([this, issue_group, &ordinal](std::string_view key, ast::NodeCPtr value) -> bool {
				if (key == "next_step_only") return true;
				bool ret = _load_issue(ordinal, key, issue_group, value);
				ordinal++;
				return ret;
			})(group_value);
		})(type_value);
	})(root);
	lock_issues();

	return ret;
}