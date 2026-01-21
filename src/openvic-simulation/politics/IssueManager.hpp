#pragma once

#include "openvic-simulation/politics/PartyPolicy.hpp"
#include "openvic-simulation/politics/Reform.hpp"
#include "openvic-simulation/core/container/IdentifierRegistry.hpp"
#include "BaseIssue.hpp"

namespace OpenVic {
	struct ConditionScript;
	struct EffectScript;
	struct ModifierManager;
	struct RuleManager;

	struct IssueManager {
	private:
		IdentifierRegistry<PartyPolicyGroup> IDENTIFIER_REGISTRY(party_policy_group);
		IdentifierRegistry<PartyPolicy> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(party_policy, party_policies);
		IdentifierRegistry<ReformType> IDENTIFIER_REGISTRY(reform_type);
		IdentifierRegistry<ReformGroup> IDENTIFIER_REGISTRY(reform_group);
		IdentifierRegistry<Reform> IDENTIFIER_REGISTRY(reform);

		bool _load_party_policy_group(size_t& expected_party_policies, std::string_view identifier, ast::NodeCPtr node);
		bool _load_party_policy(
			ModifierManager const& modifier_manager, RuleManager const& rule_manager, std::string_view identifier,
			PartyPolicyGroup& party_policy_group, ast::NodeCPtr node
		);
		bool _load_reform_group(
			size_t& expected_reforms, std::string_view identifier, ReformType& reform_type, ast::NodeCPtr node
		);
		bool _load_reform(
			ModifierManager const& modifier_manager, RuleManager const& rule_manager, size_t ordinal,
			std::string_view identifier, ReformGroup& reform_group, ast::NodeCPtr node
		);

	public:
		constexpr BaseIssue const* get_base_issue_by_identifier(std::string_view identifier) const {
			BaseIssue const* issue = get_party_policy_by_identifier(identifier);
			if (issue == nullptr) {
				issue = get_reform_by_identifier(identifier);
			}
			return issue;
		}
		constexpr BaseIssueGroup const* get_base_issue_group_by_identifier(std::string_view identifier) const {
			BaseIssueGroup const* issue = get_party_policy_group_by_identifier(identifier);
			if (issue == nullptr) {
				issue = get_reform_group_by_identifier(identifier);
			}
			return issue;
		}
		constexpr NodeTools::NodeCallback auto expect_base_issue_group_identifier(
			NodeTools::Callback<BaseIssueGroup const&> auto callback, bool warn = false
		) const {
			return NodeTools::expect_identifier(
				[this, callback, warn](std::string_view identifier) -> bool {
					if (identifier.empty()) {
						spdlog::log_s(
							warn ? spdlog::level::warn : spdlog::level::err,
							"Invalid base issue group identifier: empty!"
						);
						return warn;
					}
					BaseIssueGroup const* item = get_base_issue_group_by_identifier(identifier);
					if (item != nullptr) {
						return callback(*item);
					}
					spdlog::log_s(
						warn ? spdlog::level::warn : spdlog::level::err,
						"Invalid base issue group identifier: {}", identifier
					);
					return warn;
				}
			);
		}

		bool add_party_policy_group(std::string_view identifier);
		bool add_party_policy(
			std::string_view identifier, colour_t new_colour, ModifierValue&& values, PartyPolicyGroup& party_policy_group, RuleSet&& rules,
			bool jingoism
		);
		bool add_reform_type(std::string_view identifier, bool uncivilised);
		bool add_reform_group(std::string_view identifier, ReformType& reform_type, bool ordered, bool administrative);
		bool add_reform(
			std::string_view identifier, colour_t new_colour, ModifierValue&& values, ReformGroup& reform_group,
			size_t ordinal, fixed_point_t administrative_multiplier, RuleSet&& rules, Reform::tech_cost_t technology_cost,
			ConditionScript&& allow, ConditionScript&& on_execute_trigger, EffectScript&& on_execute_effect
		);
		bool load_issues_file(ModifierManager const& modifier_manager, RuleManager const& rule_manager, ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}