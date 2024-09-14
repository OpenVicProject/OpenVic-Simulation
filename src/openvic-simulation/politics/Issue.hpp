#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/politics/Rule.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct IssueManager;

	// Issue group (i.e. trade_policy)
	struct IssueGroup : HasIdentifier {
		friend struct IssueManager;

	protected:
		IssueGroup(std::string_view identifier);

	public:
		IssueGroup(IssueGroup&&) = default;
	};

	// Issue (i.e. protectionism)
	struct Issue : HasIdentifierAndColour, ModifierValue {
		friend struct IssueManager;

	private:
		IssueGroup const& PROPERTY(group);
		RuleSet PROPERTY(rules);
		const bool PROPERTY_CUSTOM_PREFIX(jingoism, is);

	protected:
		Issue(
			std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_values, IssueGroup const& new_group,
			RuleSet&& new_rules, bool new_jingoism
		);

	public:
		Issue(Issue&&) = default;
	};

	// Reform type (i.e. political_issues)
	struct ReformType : HasIdentifier {
		friend struct IssueManager;

	private:
		bool PROPERTY_CUSTOM_PREFIX(uncivilised, is); // whether this group is available to non-westernised countries
		// in vanilla education, military and economic reforms are hardcoded to true and the rest to false

		ReformType(std::string_view new_identifier, bool new_uncivilised);

	public:
		ReformType(ReformType&&) = default;
	};

	// Reform group (i.e. slavery)
	struct ReformGroup : IssueGroup {
		friend struct IssueManager;

	private:
		ReformType const& PROPERTY(type);
		const bool PROPERTY_CUSTOM_PREFIX(ordered, is); // next_step_only
		const bool PROPERTY_CUSTOM_PREFIX(administrative, is);

		ReformGroup(std::string_view new_identifier, ReformType const& new_type, bool new_ordered, bool new_administrative);

	public:
		ReformGroup(ReformGroup&&) = default;
	};

	// Reform (i.e. yes_slavery)
	struct Reform : Issue {
		friend struct IssueManager;
		using tech_cost_t = uint32_t;

	private:
		ReformGroup const& PROPERTY(reform_group); // stores an already casted reference
		const size_t PROPERTY(ordinal); // assigned by the parser to allow policy sorting
		const fixed_point_t PROPERTY(administrative_multiplier);
		const tech_cost_t PROPERTY(technology_cost);
		ConditionScript PROPERTY(allow);
		ConditionScript PROPERTY(on_execute_trigger);
		EffectScript PROPERTY(on_execute_effect);

		Reform(
			std::string_view new_identifier, colour_t new_colour, ModifierValue&& new_values, ReformGroup const& new_group,
			size_t new_ordinal, fixed_point_t new_administrative_multiplier, RuleSet&& new_rules,
			tech_cost_t new_technology_cost, ConditionScript&& new_allow, ConditionScript&& new_on_execute_trigger,
			EffectScript&& new_on_execute_effect
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		Reform(Reform&&) = default;
	};

	// Issue manager - holds the registries
	struct IssueManager {
	private:
		IdentifierRegistry<IssueGroup> IDENTIFIER_REGISTRY(issue_group);
		IdentifierRegistry<Issue> IDENTIFIER_REGISTRY(issue);
		IdentifierRegistry<ReformType> IDENTIFIER_REGISTRY(reform_type);
		IdentifierRegistry<ReformGroup> IDENTIFIER_REGISTRY(reform_group);
		IdentifierRegistry<Reform> IDENTIFIER_REGISTRY(reform);

		bool _load_issue_group(size_t& expected_issues, std::string_view identifier, ast::NodeCPtr node);
		bool _load_issue(
			ModifierManager const& modifier_manager, RuleManager const& rule_manager, std::string_view identifier,
			IssueGroup const* group, ast::NodeCPtr node
		);
		bool _load_reform_group(
			size_t& expected_reforms, std::string_view identifier, ReformType const* type, ast::NodeCPtr node
		);
		bool _load_reform(
			ModifierManager const& modifier_manager, RuleManager const& rule_manager, size_t ordinal,
			std::string_view identifier, ReformGroup const* group, ast::NodeCPtr node
		);

	public:
		bool add_issue_group(std::string_view identifier);
		bool add_issue(
			std::string_view identifier, colour_t new_colour, ModifierValue&& values, IssueGroup const* group, RuleSet&& rules,
			bool jingoism
		);
		bool add_reform_type(std::string_view identifier, bool uncivilised);
		bool add_reform_group(std::string_view identifier, ReformType const* type, bool ordered, bool administrative);
		bool add_reform(
			std::string_view identifier, colour_t new_colour, ModifierValue&& values, ReformGroup const* group, size_t ordinal,
			fixed_point_t administrative_multiplier, RuleSet&& rules, Reform::tech_cost_t technology_cost,
			ConditionScript&& allow, ConditionScript&& on_execute_trigger, EffectScript&& on_execute_effect
		);
		bool load_issues_file(ModifierManager const& modifier_manager, RuleManager const& rule_manager, ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
