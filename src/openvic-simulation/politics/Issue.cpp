#include "Issue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

IssueGroup::IssueGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Issue::Issue(std::string_view identifier, IssueGroup const& group)
	: HasIdentifier { identifier }, group { group } {}

IssueGroup const& Issue::get_group() const {
	return group;
}

ReformType::ReformType(std::string_view new_identifier, bool uncivilised)
	: HasIdentifier { new_identifier }, uncivilised { uncivilised } {}

ReformGroup::ReformGroup(std::string_view identifier, ReformType const& type, bool ordered, bool administrative)
	: IssueGroup { identifier }, type { type }, ordered { ordered }, administrative { administrative } {}

ReformType const& ReformGroup::get_type() const {
	return type;
}

bool ReformGroup::is_ordered() const {
	return ordered;
}

bool ReformGroup::is_administrative() const {
	return administrative;
}

Reform::Reform(std::string_view identifier, ReformGroup const& group, size_t ordinal)
	: Issue { identifier, group }, ordinal { ordinal }, reform_group { group } {}

ReformGroup const& Reform::get_reform_group() const {
	return reform_group;
}

ReformType const& Reform::get_type() const {
	return get_reform_group().get_type();
}

size_t Reform::get_ordinal() const {
	return ordinal;
}

IssueManager::IssueManager() : issue_groups { "issue groups" }, issues { "issues" },
	reform_types { "reform types" }, reform_groups { "reform groups" }, reforms { "reforms" }  {}

bool IssueManager::add_issue_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid issue group identifier - empty!");
		return false;
	}

	return issue_groups.add_item({ identifier });
}

bool IssueManager::add_issue(std::string_view identifier, IssueGroup const* group) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null issue group for ", identifier);
		return false;
	}

	return issues.add_item({ identifier, *group });
}

bool IssueManager::add_reform_type(std::string_view identifier, bool uncivilised) {
	if (identifier.empty()) {
		Logger::error("Invalid issue type identifier - empty!");
		return false;
	}

	return reform_types.add_item({ identifier, uncivilised });
}

bool IssueManager::add_reform_group(std::string_view identifier, ReformType const* type, bool ordered, bool administrative) {
	if (identifier.empty()) {
		Logger::error("Invalid issue group identifier - empty!");
		return false;
	}

	if (type == nullptr) {
		Logger::error("Null issue type for ", identifier);
		return false;
	}

	return reform_groups.add_item({ identifier, *type, ordered, administrative });
}

bool IssueManager::add_reform(std::string_view identifier, ReformGroup const* group, size_t ordinal) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null issue group for ", identifier);
		return false;
	}

	return reforms.add_item({ identifier, *group, ordinal });
}

bool IssueManager::_load_issue_group(size_t& expected_issues, std::string_view identifier, ast::NodeCPtr node) {
	return expect_length([&expected_issues](size_t size) -> size_t {
		expected_issues += size;
		return size;
	})(node) & add_issue_group(identifier);
}

bool IssueManager::_load_issue(std::string_view identifier, IssueGroup const* group, ast::NodeCPtr node) {
	//TODO: policy modifiers, policy rule changes
	return add_issue(identifier, group);
}

bool IssueManager::_load_reform_group(size_t& expected_reforms, std::string_view identifier, ReformType const* type, ast::NodeCPtr node) {
	bool ordered = false, administrative = false;
	bool ret = expect_dictionary_keys_and_default(
		increment_callback(expected_reforms),
		"next_step_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(ordered)),
		"administrative", ZERO_OR_ONE, expect_bool(assign_variable_callback(administrative))
	)(node);
	ret &= add_reform_group(identifier, type, ordered, administrative);
	return ret;
}

bool IssueManager::_load_reform(size_t& ordinal, std::string_view identifier, ReformGroup const* group, ast::NodeCPtr node) {
	//TODO: conditions to allow, policy modifiers, policy rule changes
	return add_reform(identifier, group, ordinal);
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
	size_t expected_reform_groups = 0;
	bool ret = expect_dictionary_reserve_length(reform_types,
		[this, &expected_issue_groups, &expected_reform_groups](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "party_issues")
				return expect_length(add_variable_callback(expected_issue_groups))(value);
			else return expect_length(add_variable_callback(expected_reform_groups))(value) & add_reform_type(
				key, key == "economic_reforms" || "education_reforms" || "military_reforms");
		}
	)(root);
	lock_reform_types();

	issue_groups.reserve(issue_groups.size() + expected_issue_groups);
	reform_groups.reserve(reform_groups.size() + expected_reform_groups);

	size_t expected_issues = 0;
	size_t expected_reforms = 0;
	ret &= expect_dictionary_reserve_length(issue_groups,
		[this, &expected_issues, &expected_reforms](std::string_view type_key, ast::NodeCPtr type_value) -> bool {
			if (type_key == "party_issues") {
				return expect_dictionary([this, &expected_issues](std::string_view key, ast::NodeCPtr value) -> bool {
					return _load_issue_group(expected_issues, key, value);
				})(type_value);
			} else {
				ReformType const* reform_type = get_reform_type_by_identifier(type_key);
				return expect_dictionary(
					[this, reform_type, &expected_reforms](std::string_view key, ast::NodeCPtr value) -> bool {
						return _load_reform_group(expected_reforms, key, reform_type, value);
					}
				)(type_value);
			}
		}
	)(root);
	lock_issue_groups();
	lock_reform_groups();

	issues.reserve(issues.size() + expected_issues);
	reforms.reserve(reforms.size() + expected_reforms);

	ret &= expect_dictionary([this](std::string_view type_key, ast::NodeCPtr type_value) -> bool {
		return expect_dictionary([this, type_key](std::string_view group_key, ast::NodeCPtr group_value) -> bool {
			if (type_key == "party_issues") {
				IssueGroup const* issue_group = get_issue_group_by_identifier(group_key);
				return expect_dictionary([this, issue_group](std::string_view key, ast::NodeCPtr value) -> bool {
					bool ret = _load_issue(key, issue_group, value);
					return ret;
				})(group_value);
			} else {
				ReformGroup const* reform_group = get_reform_group_by_identifier(group_key);
				size_t ordinal = 0;
				return expect_dictionary([this, reform_group, &ordinal](std::string_view key, ast::NodeCPtr value) -> bool {
					if (key == "next_step_only" || key == "administrative") return true;
					bool ret = _load_reform(ordinal, key, reform_group, value);
					ordinal++;
					return ret;
				})(group_value);
			}
		})(type_value);
	})(root);
	lock_issues();
	lock_reforms();

	return ret;
}