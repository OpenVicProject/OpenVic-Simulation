#include "IssueManager.hpp"

#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/utility/LogScope.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool IssueManager::add_party_policy_group(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid party policy group identifier - empty!");
		return false;
	}

	return party_policy_groups.emplace_item(
		identifier,
		identifier, get_party_policy_group_count()
	);
}

bool IssueManager::add_party_policy(
	std::string_view identifier, colour_t new_colour, ModifierValue&& values, PartyPolicyGroup& party_policy_group, RuleSet&& rules,
	bool jingoism
) {
	if (identifier.empty()) {
		Logger::error("Invalid party policy identifier - empty!");
		return false;
	}

	if (!party_policies.emplace_item(
		identifier,
		get_party_policy_count(), identifier,
		new_colour, std::move(values), party_policy_group, std::move(rules), jingoism
	)) {
		return false;
	}

	party_policy_group.issues.push_back(&get_back_party_policy());
	return true;
}

bool IssueManager::add_reform_type(std::string_view identifier, bool uncivilised) {
	if (identifier.empty()) {
		Logger::error("Invalid issue type identifier - empty!");
		return false;
	}

	return reform_types.emplace_item(
		identifier,
		identifier, uncivilised
	);
}

bool IssueManager::add_reform_group(
	std::string_view identifier, ReformType& reform_type, bool ordered, bool administrative
) {
	if (identifier.empty()) {
		Logger::error("Invalid issue group identifier - empty!");
		return false;
	}

	if (!reform_groups.emplace_item(
		identifier,
		identifier, get_reform_group_count(), reform_type, ordered, administrative
	)) {
		return false;
	}

	reform_type.reform_groups.push_back(&get_back_reform_group());
	return true;
}

bool IssueManager::add_reform(
	std::string_view identifier, colour_t new_colour, ModifierValue&& values, ReformGroup& reform_group, size_t ordinal,
	fixed_point_t administrative_multiplier, RuleSet&& rules, Reform::tech_cost_t technology_cost, ConditionScript&& allow,
	ConditionScript&& on_execute_trigger, EffectScript&& on_execute_effect
) {
	if (identifier.empty()) {
		Logger::error("Invalid issue identifier - empty!");
		return false;
	}

	if (reform_group.is_uncivilised()) {
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

	if (administrative_multiplier != 0 && !reform_group.get_is_administrative()) {
		Logger::warning(
			"Non-zero administrative multiplier ", administrative_multiplier, " found in reform ", identifier,
			" belonging to non-administrative group ", reform_group.get_identifier(), "!"
		);
	}

	if (!reforms.emplace_item(
		identifier,
		get_reform_count(), identifier,
		new_colour, std::move(values), reform_group, ordinal, administrative_multiplier, std::move(rules),
		technology_cost, std::move(allow), std::move(on_execute_trigger), std::move(on_execute_effect)
	)) {
		return false;
	}

	reform_group.issues.push_back(&get_back_reform());
	return true;
}

bool IssueManager::_load_party_policy_group(size_t& expected_party_policies, std::string_view identifier, ast::NodeCPtr node) {
	const LogScope log_scope { fmt::format("party policy group {}", identifier) };
	return expect_length(add_variable_callback(expected_party_policies))(node)
		& add_party_policy_group(identifier);
}

/* Each colour is made up of these components in some order:
 *  - 0
 *  - floor(max_value * 0.99) = 252
 *  - floor(max_value * i / 60) = floor(4.25 * i) for some i in [0, 60) */

static constexpr colour_t::value_type dim_colour_component = 0;
static constexpr colour_t::value_type bright_colour_component = static_cast<uint32_t>(colour_t::max_value) * 99 / 100;

/* Prime factor to scatter [0, 60) */
static constexpr size_t scattering_prime = 4057;
static constexpr size_t varying_colour_denominator = 60;

/* 3! = 3 (choices for 0) * 2 (choices for 252) * 1 (choices for 4.25 * i) */
static constexpr size_t colour_pattern_period = 6;

static constexpr std::array<size_t, colour_pattern_period> dim_colour_indices     { 0, 1, 2, 0, 1, 2 };
static constexpr std::array<size_t, colour_pattern_period> bright_colour_indices  { 1, 2, 0, 2, 0, 1 };
static constexpr std::array<size_t, colour_pattern_period> varying_colour_indices { 2, 0, 1, 1, 2, 0 };

static constexpr colour_t create_issue_reform_colour(size_t index) {
	colour_t ret {};

	const size_t periodic_index = index % colour_pattern_period;

	ret[dim_colour_indices[periodic_index]] = dim_colour_component;

	ret[bright_colour_indices[periodic_index]] = bright_colour_component;

	ret[varying_colour_indices[periodic_index]] = static_cast<size_t>(colour_t::max_value)
		* ((index * scattering_prime) % varying_colour_denominator) / varying_colour_denominator;

	return ret;
}

bool IssueManager::_load_party_policy(
	ModifierManager const& modifier_manager, RuleManager const& rule_manager, std::string_view identifier,
	PartyPolicyGroup& party_policy_group, ast::NodeCPtr node
) {
	const LogScope log_scope { fmt::format("party policy {}", identifier) };

	ModifierValue values;
	RuleSet rules;
	bool is_jingoism = false;

	bool ret = NodeTools::expect_dictionary_keys_and_default(
		modifier_manager.expect_base_country_modifier(values),
		"is_jingoism", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_jingoism)),
		"rules", ZERO_OR_ONE, rule_manager.expect_rule_set(move_variable_callback(rules)),
		"war_exhaustion_effect", ZERO_OR_ONE, [](const ast::NodeCPtr _) -> bool {
			Logger::warning("war_exhaustion_effect does nothing (vanilla issues have it).");
			return true;
		}
	)(node);

	ret &= add_party_policy(
		identifier, create_issue_reform_colour(get_party_policy_count() + get_reform_count()), std::move(values), party_policy_group,
		std::move(rules), is_jingoism
	);

	return ret;
}

bool IssueManager::_load_reform_group(
	size_t& expected_reforms, std::string_view identifier, ReformType& reform_type, ast::NodeCPtr node
) {
	const LogScope log_scope { fmt::format("reform group {}", identifier) };
	bool ordered = false, administrative = false;

	bool ret = expect_dictionary_keys_and_default(
		increment_callback(expected_reforms),
		"next_step_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(ordered)),
		"administrative", ZERO_OR_ONE, expect_bool(assign_variable_callback(administrative))
	)(node);

	ret &= add_reform_group(identifier, reform_type, ordered, administrative);

	return ret;
}

bool IssueManager::_load_reform(
	ModifierManager const& modifier_manager, RuleManager const& rule_manager, size_t ordinal, std::string_view identifier,
	ReformGroup& reform_group, ast::NodeCPtr node
) {
	const LogScope log_scope { fmt::format("reform {}", identifier) };
	using enum scope_type_t;

	ModifierValue values;
	RuleSet rules;
	fixed_point_t administrative_multiplier = 0;
	Reform::tech_cost_t technology_cost = 0;
	ConditionScript allow { COUNTRY, COUNTRY, NO_SCOPE };
	ConditionScript on_execute_trigger { COUNTRY, COUNTRY, NO_SCOPE };
	EffectScript on_execute_effect;

	bool ret = NodeTools::expect_dictionary_keys_and_default(
		modifier_manager.expect_base_country_modifier(values),
		"administrative_multiplier", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(administrative_multiplier)),
		"technology_cost", ZERO_OR_ONE, expect_uint(assign_variable_callback(technology_cost)),
		"allow", ZERO_OR_MORE, allow.expect_script(),
		"rules", ZERO_OR_ONE, rule_manager.expect_rule_set(move_variable_callback(rules)),
		"on_execute", ZERO_OR_ONE, expect_dictionary_keys(
			"trigger", ZERO_OR_ONE, on_execute_trigger.expect_script(),
			"effect", ONE_EXACTLY, on_execute_effect.expect_script()
		)
	)(node);

	ret &= add_reform(
		identifier, create_issue_reform_colour(get_party_policy_count() + get_reform_count()), std::move(values), reform_group,
		ordinal, administrative_multiplier, std::move(rules), technology_cost, std::move(allow), std::move(on_execute_trigger),
		std::move(on_execute_effect)
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
bool IssueManager::load_issues_file(
	ModifierManager const& modifier_manager, RuleManager const& rule_manager, ast::NodeCPtr root
) {
	const LogScope log_scope { "common/issues.txt" };
	bool party_issues_found = false;
	size_t expected_party_policy_groups = 0;
	size_t expected_reform_groups = 0;

	/* Reserve space for issue and reform groups and reserve space for and load reform types. */
	bool ret = expect_dictionary_reserve_length(
		reform_types,
		[this, &party_issues_found, &expected_party_policy_groups, &expected_reform_groups](
			std::string_view key, ast::NodeCPtr value
		) -> bool {
			if (key == "party_issues") {
				if (party_issues_found) {
					/* Error emitted here allowing later passes to return true with no error message. */
					Logger::error("Duplicate party issues found!");
					return false;
				}
				party_issues_found = true;

				return expect_length(add_variable_callback(expected_party_policy_groups))(value);
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

	reserve_more_party_policy_groups(expected_party_policy_groups);
	reserve_more_reform_groups(expected_reform_groups);

	party_issues_found = false;
	size_t expected_party_policies = 0;
	size_t expected_reforms = 0;

	/* Load issue and reform groups. */
	ret &= expect_dictionary(
		[this, &party_issues_found, &expected_party_policies, &expected_reforms](
			std::string_view type_key, ast::NodeCPtr type_value
		) -> bool {
			if (type_key == "party_issues") {
				if (party_issues_found) {
					return true; /* Error will have been emitted in first pass. */
				}
				party_issues_found = true;

				return expect_dictionary([this, &expected_party_policies](std::string_view key, ast::NodeCPtr value) -> bool {
					return _load_party_policy_group(expected_party_policies, key, value);
				})(type_value);
			} else {
				ReformType* reform_type = reform_types.get_item_by_identifier(type_key);

				if (OV_unlikely(reform_type == nullptr)) {
					Logger::error("Reform type \"", type_key, "\" not found!");
					return false;
				}

				return expect_dictionary(
					[this, reform_type, &expected_reforms](std::string_view key, ast::NodeCPtr value) -> bool {
						return _load_reform_group(expected_reforms, key, *reform_type, value);
					}
				)(type_value);
			}
		}
	)(root);

	lock_party_policy_groups();
	lock_reform_groups();

	reserve_more_party_policies(expected_party_policies);
	reserve_more_reforms(expected_reforms);

	party_issues_found = false;

	/* Load issues and reforms. */
	ret &= expect_dictionary(
		[this, &party_issues_found, &modifier_manager, &rule_manager](
			std::string_view type_key, ast::NodeCPtr type_value
		) -> bool {
			if (type_key == "party_issues") {
				if (party_issues_found) {
					return true;
				}
				party_issues_found = true;

				return expect_dictionary([this, &modifier_manager, &rule_manager](
					std::string_view group_key, ast::NodeCPtr group_value
				) -> bool {
					PartyPolicyGroup* party_policy_group = party_policy_groups.get_item_by_identifier(group_key);

					if (OV_unlikely(party_policy_group == nullptr)) {
						Logger::error("Party policy group \"", group_key, "\" not found!");
						return false;
					}

					return expect_dictionary([this, &modifier_manager, &rule_manager, party_policy_group](
						std::string_view key, ast::NodeCPtr value
					) -> bool {
						return _load_party_policy(modifier_manager, rule_manager, key, *party_policy_group, value);
					})(group_value);
				})(type_value);
			} else {
				return expect_dictionary([this, &party_issues_found, &modifier_manager, &rule_manager](
					std::string_view group_key, ast::NodeCPtr group_value
				) -> bool {
					ReformGroup* reform_group = reform_groups.get_item_by_identifier(group_key);

					if (OV_unlikely(reform_group == nullptr)) {
						Logger::error("Reform group \"", group_key, "\" not found!");
						return false;
					}

					size_t ordinal = 0;

					return expect_dictionary([this, &modifier_manager, &rule_manager, reform_group, &ordinal](
						std::string_view key, ast::NodeCPtr value
					) -> bool {
						if (key == "next_step_only" || key == "administrative") {
							return true;
						}

						return _load_reform(modifier_manager, rule_manager, ordinal++, key, *reform_group, value);
					})(group_value);
				})(type_value);
			}
		}
	)(root);

	lock_party_policies();
	lock_reforms();

	return ret;
}

bool IssueManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (Reform& reform : reforms.get_items()) {
		ret &= reform.parse_scripts(definition_manager);
	}

	return ret;
}
