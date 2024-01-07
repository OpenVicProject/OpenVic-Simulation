#include "Issue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

IssueGroup::IssueGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Issue::Issue(std::string_view new_identifier, ModifierValue&& new_values, IssueGroup const& new_group, RuleSet&& new_rules, bool new_jingoism)
	: Modifier { new_identifier, std::move(new_values), 0 }, group { new_group }, rules { std::move(new_rules) },
	jingoism { new_jingoism } {}

ReformType::ReformType(std::string_view new_identifier, bool new_uncivilised)
	: HasIdentifier { new_identifier }, uncivilised { new_uncivilised } {}

ReformGroup::ReformGroup(std::string_view new_identifier, ReformType const& new_type, bool new_ordered, bool new_administrative)
	: IssueGroup { new_identifier }, type { new_type }, ordered { new_ordered }, administrative { new_administrative } {}

Reform::Reform(
	std::string_view new_identifier, ModifierValue&& new_values, ReformGroup const& new_group, size_t new_ordinal,
	RuleSet&& new_rules, tech_cost_t new_technology_cost, ConditionScript&& new_allow,
	ConditionScript&& new_on_execute_trigger, EffectScript&& new_on_execute_effect
) : Issue { new_identifier, std::move(new_values), new_group, std::move(new_rules), false }, ordinal { new_ordinal },
	reform_group { new_group }, technology_cost { new_technology_cost }, allow { std::move(new_allow) },
	on_execute_trigger { std::move(new_on_execute_trigger) }, on_execute_effect { std::move(new_on_execute_effect) } {}

bool Reform::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	ret &= allow.parse_script(true, game_manager);
	ret &= on_execute_trigger.parse_script(true, game_manager);
	ret &= on_execute_effect.parse_script(true, game_manager);
	return ret;
}

bool IssueManager::add_issue_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid issue group identifier - empty!");
		return false;
	}

	return issue_groups.add_item({ identifier });
}

bool IssueManager::add_issue(
	std::string_view identifier, ModifierValue&& values, IssueGroup const* group, RuleSet&& rules, bool jingoism
) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null issue group for ", identifier);
		return false;
	}

	return issues.add_item({ identifier, std::move(values), *group, std::move(rules), jingoism });
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

bool IssueManager::add_reform(
	std::string_view identifier, ModifierValue&& values, ReformGroup const* group, size_t ordinal, RuleSet&& rules,
	Reform::tech_cost_t technology_cost, ConditionScript&& allow, ConditionScript&& on_execute_trigger,
	EffectScript&& on_execute_effect
) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null issue group for ", identifier);
		return false;
	}

	if (group->get_type().is_uncivilised()) {
		if (ordinal == 0) {
			if (technology_cost != 0) {
				Logger::warning(
					"Non-zero technology cost ", technology_cost, " found in ordinal 0 uncivilised reform ", identifier, "!"
				);
			}
		} else {
			if (technology_cost <= 0) {
				Logger::warning(
					"Non-positive technology cost ", technology_cost, " found in ordinal ", ordinal,
					" uncivilised reform ", identifier, "!"
				);
			}
		}
	} else if (technology_cost != 0) {
		Logger::warning("Non-zero technology cost ", technology_cost, " found in civilised reform ", identifier, "!");
	}

	return reforms.add_item({
		identifier, std::move(values), *group, ordinal, std::move(rules), technology_cost, std::move(allow),
		std::move(on_execute_trigger), std::move(on_execute_effect)
	});
}

bool IssueManager::_load_issue_group(size_t& expected_issues, std::string_view identifier, ast::NodeCPtr node) {
	return expect_length(add_variable_callback(expected_issues))(node)
		& add_issue_group(identifier);
}

bool IssueManager::_load_issue(
	ModifierManager const& modifier_manager, RuleManager const& rule_manager, std::string_view identifier,
	IssueGroup const* group, ast::NodeCPtr node
) {
	ModifierValue values;
	RuleSet rules;
	bool jingoism = false;
	bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(values),
		"is_jingoism", ZERO_OR_ONE, expect_bool(assign_variable_callback(jingoism)),
		"rules", ZERO_OR_ONE, rule_manager.expect_rule_set(move_variable_callback(rules))
	)(node);
	ret &= add_issue(identifier, std::move(values), group, std::move(rules), jingoism);
	return ret;
}

bool IssueManager::_load_reform_group(
	size_t& expected_reforms, std::string_view identifier, ReformType const* type, ast::NodeCPtr node
) {
	bool ordered = false, administrative = false;
	bool ret = expect_dictionary_keys_and_default(
		increment_callback(expected_reforms),
		"next_step_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(ordered)),
		"administrative", ZERO_OR_ONE, expect_bool(assign_variable_callback(administrative))
	)(node);
	ret &= add_reform_group(identifier, type, ordered, administrative);
	return ret;
}

bool IssueManager::_load_reform(
	ModifierManager const& modifier_manager, RuleManager const& rule_manager, size_t ordinal, std::string_view identifier,
	ReformGroup const* group, ast::NodeCPtr node
) {
	ModifierValue values;
	RuleSet rules;
	Reform::tech_cost_t technology_cost = 0;
	ConditionScript allow { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
	ConditionScript on_execute_trigger { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };
	EffectScript on_execute_effect;

	bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(values),
		"technology_cost", ZERO_OR_ONE, expect_uint(assign_variable_callback(technology_cost)),
		"allow", ZERO_OR_ONE, allow.expect_script(),
		"rules", ZERO_OR_ONE, rule_manager.expect_rule_set(move_variable_callback(rules)),
		"on_execute", ZERO_OR_ONE, expect_dictionary_keys(
			"trigger", ZERO_OR_ONE, on_execute_trigger.expect_script(),
			"effect", ONE_EXACTLY, on_execute_effect.expect_script()
		)
	)(node);
	ret &= add_reform(
		identifier, std::move(values), group, ordinal, std::move(rules), technology_cost, std::move(allow),
		std::move(on_execute_trigger), std::move(on_execute_effect)
	);
	return ret;
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
bool IssueManager::load_issues_file(ModifierManager const& modifier_manager, RuleManager const& rule_manager, ast::NodeCPtr root) {
	size_t expected_issue_groups = 0;
	size_t expected_reform_groups = 0;
	bool ret = expect_dictionary_reserve_length(reform_types,
		[this, &expected_issue_groups, &expected_reform_groups](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "party_issues") {
				return expect_length(add_variable_callback(expected_issue_groups))(value);
			} else {
				static const string_set_t uncivilised_reform_groups {
					"economic_reforms", "education_reforms", "military_reforms"
				};
				return expect_length(add_variable_callback(expected_reform_groups))(value)
					& add_reform_type(key, uncivilised_reform_groups.contains(key));
			}
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

	ret &= expect_dictionary([this, &modifier_manager, &rule_manager](std::string_view type_key, ast::NodeCPtr type_value) -> bool {
		return expect_dictionary([this, &modifier_manager, &rule_manager, type_key](std::string_view group_key, ast::NodeCPtr group_value) -> bool {
			if (type_key == "party_issues") {
				IssueGroup const* issue_group = get_issue_group_by_identifier(group_key);
				return expect_dictionary([this, &modifier_manager, &rule_manager, issue_group](std::string_view key, ast::NodeCPtr value) -> bool {
					return _load_issue(modifier_manager, rule_manager, key, issue_group, value);
				})(group_value);
			} else {
				ReformGroup const* reform_group = get_reform_group_by_identifier(group_key);
				size_t ordinal = 0;
				return expect_dictionary([this, &modifier_manager, &rule_manager, reform_group, &ordinal](std::string_view key, ast::NodeCPtr value) -> bool {
					if (key == "next_step_only" || key == "administrative") {
						return true;
					}
					return _load_reform(modifier_manager, rule_manager, ordinal++, key, reform_group, value);
				})(group_value);
			}
		})(type_value);
	})(root);
	lock_issues();
	lock_reforms();

	return ret;
}

bool IssueManager::parse_scripts(GameManager& game_manager) {
	bool ret = true;
	for (Reform& reform : reforms.get_items()) {
		ret &= reform.parse_scripts(game_manager);
	}
	return ret;
}
