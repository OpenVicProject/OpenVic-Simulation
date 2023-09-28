#pragma once

#include <cstddef>
#include <string_view>
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-dataloader/v2script/AbstractSyntaxTree.hpp"

namespace OpenVic {
	struct IssueManager;

	//Issue group (i.e. trade_policy)
	struct IssueGroup : HasIdentifier {
		friend struct IssueManager;

	protected:
		IssueGroup(std::string_view identifier);

	public:
		IssueGroup(IssueGroup&&) = default;
	};

	//Issue (i.e. protectionism)
	struct Issue : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueGroup const& group;

		//TODO: policy modifiers, policy rule changes

	protected:
		Issue(std::string_view identifier, IssueGroup const& group);

	public:
		Issue(Issue&&) = default;
		IssueGroup const& get_group() const;
	};

	//Reform type (i.e. political_issues)
	struct ReformType : HasIdentifier {
		friend struct IssueManager;

	private:
		bool uncivilised; //whether this group is available to non-westernised countries
		//in vanilla education, military and economic reforms are hardcoded to true and the rest to false

		ReformType(std::string_view new_identifier, bool uncivilised);

	public:
		ReformType(ReformType&&) = default;
	};

	//Reform group (i.e. slavery)
	struct ReformGroup : IssueGroup {
		friend struct IssueManager;

	private:
		ReformType const& type;
		const bool ordered; //next_step_only
		const bool administrative;

		ReformGroup(std::string_view identifier, ReformType const& type, bool ordered, bool administrative);

	public:
		ReformGroup(ReformGroup&&) = default;
		ReformType const& get_type() const;
		bool is_ordered() const;
		bool is_administrative() const;
	};

	//Reform (i.e. yes_slavery)
	struct Reform : Issue {
		friend struct IssueManager;

	private:
		ReformGroup const& reform_group; //stores an already casted reference
		const size_t ordinal; //assigned by the parser to allow policy sorting

		Reform(std::string_view new_identifier, ReformGroup const& group, size_t ordinal);

		//TODO: conditions to allow,

	public:
		Reform(Reform&&) = default;
		ReformGroup const& get_reform_group() const;
		ReformType const& get_type() const;
		size_t get_ordinal() const;
	};

	//Issue manager - holds the registries
	struct IssueManager {
	private:
		IdentifierRegistry<IssueGroup> issue_groups;
		IdentifierRegistry<Issue> issues;
		IdentifierRegistry<ReformType> reform_types;
		IdentifierRegistry<ReformGroup> reform_groups;
		IdentifierRegistry<Reform> reforms;

		bool _load_issue_group(size_t& expected_issues, std::string_view identifier, ast::NodeCPtr node);
		bool _load_issue(std::string_view identifier, IssueGroup const* group, ast::NodeCPtr node);
		bool _load_reform_group(size_t& expected_reforms, std::string_view identifier, ReformType const* type,
			ast::NodeCPtr node);
		bool _load_reform(size_t& ordinal, std::string_view identifier, ReformGroup const* group, ast::NodeCPtr node);

	public:
		IssueManager();

		bool add_issue_group(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(IssueGroup, issue_group)

		bool add_issue(std::string_view identifier, IssueGroup const* group);
		IDENTIFIER_REGISTRY_ACCESSORS(Issue, issue)

		bool add_reform_type(std::string_view identifier, bool uncivilised);
		IDENTIFIER_REGISTRY_ACCESSORS(ReformType, reform_type)

		bool add_reform_group(std::string_view identifier, ReformType const* type, bool ordered, bool administrative);
		IDENTIFIER_REGISTRY_ACCESSORS(ReformGroup, reform_group)

		bool add_reform(std::string_view identifier, ReformGroup const* group, size_t ordinal);
		IDENTIFIER_REGISTRY_ACCESSORS(Reform, reform)

		bool load_issues_file(ast::NodeCPtr root);
	};
}