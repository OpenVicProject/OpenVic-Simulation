#pragma once

#include <cstddef>
#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
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
	struct Issue : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueGroup const& PROPERTY(group);

		// TODO: policy modifiers, policy rule changes

	protected:
		Issue(std::string_view identifier, IssueGroup const& group);

	public:
		Issue(Issue&&) = default;
	};

	// Reform type (i.e. political_issues)
	struct ReformType : HasIdentifier {
		friend struct IssueManager;

	private:
		bool uncivilised; // whether this group is available to non-westernised countries
		// in vanilla education, military and economic reforms are hardcoded to true and the rest to false

		ReformType(std::string_view new_identifier, bool uncivilised);

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

		ReformGroup(std::string_view identifier, ReformType const& type, bool ordered, bool administrative);

	public:
		ReformGroup(ReformGroup&&) = default;
	};

	// Reform (i.e. yes_slavery)
	struct Reform : Issue {
		friend struct IssueManager;

	private:
		ReformGroup const& PROPERTY(reform_group); // stores an already casted reference
		const size_t PROPERTY(ordinal); // assigned by the parser to allow policy sorting

		Reform(std::string_view new_identifier, ReformGroup const& group, size_t ordinal);

		// TODO: conditions to allow,

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
		bool _load_issue(std::string_view identifier, IssueGroup const* group, ast::NodeCPtr node);
		bool _load_reform_group(
			size_t& expected_reforms, std::string_view identifier, ReformType const* type, ast::NodeCPtr node
		);
		bool _load_reform(size_t& ordinal, std::string_view identifier, ReformGroup const* group, ast::NodeCPtr node);

	public:
		bool add_issue_group(std::string_view identifier);

		bool add_issue(std::string_view identifier, IssueGroup const* group);

		bool add_reform_type(std::string_view identifier, bool uncivilised);

		bool add_reform_group(std::string_view identifier, ReformType const* type, bool ordered, bool administrative);

		bool add_reform(std::string_view identifier, ReformGroup const* group, size_t ordinal);

		bool load_issues_file(ast::NodeCPtr root);
	};
}
