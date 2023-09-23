#pragma once

#include <cstddef>
#include <string_view>
#include "types/IdentifierRegistry.hpp"
#include "dataloader/NodeTools.hpp"

namespace OpenVic {
	struct IssueManager;

	//Issue type (i.e. political_issues)
	struct IssueType : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueType(const std::string_view new_identifier);
	
	public:
		IssueType(IssueType&&) = default;
	};

	//Issue group (i.e. slavery)
	struct IssueGroup : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueType const& type;
		const bool ordered, administrative;

		IssueGroup(const std::string_view new_identifier, IssueType const& new_type, bool new_ordered, bool new_administrative);
	
	public:
		IssueGroup(IssueGroup&&) = default;
		IssueType const& get_type() const;
		bool is_ordered() const;
		bool is_administrative() const;
	};

	//Issue type (i.e. yes_slavery)
	struct Issue : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueGroup const& group;
		const size_t ordinal; //assigned by the parser to allow policy sorting

		//TODO: conditions to allow, policy modifiers, policy rule changes

		Issue(const std::string_view new_identifier, IssueGroup const& new_group, size_t ordinal);

	public:
		Issue(Issue&&) = default;
		IssueType const& get_type() const;
		IssueGroup const& get_group() const;
		size_t get_ordinal() const;
	};
	
	//Issue manager - holds the registries
	struct IssueManager {
	private:
		IdentifierRegistry<IssueType> issue_types;
		IdentifierRegistry<IssueGroup> issue_groups;
		IdentifierRegistry<Issue> issues;

		bool _load_issue_group(size_t& expected_issues, const std::string_view identifier, IssueType const* type, ast::NodeCPtr node);
		bool _load_issue(size_t& ordinal, const std::string_view identifier, IssueGroup const* group, ast::NodeCPtr node);

	public:
		IssueManager();
		
		bool add_issue_type(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(IssueType, issue_type)

		bool add_issue_group(const std::string_view identifier, IssueType const* type, bool ordered, bool administrative);
		IDENTIFIER_REGISTRY_ACCESSORS(IssueGroup, issue_group)

		bool add_issue(const std::string_view identifier, IssueGroup const* group, size_t ordinal);
		IDENTIFIER_REGISTRY_ACCESSORS(Issue, issue)

		bool load_issues_file(ast::NodeCPtr root);
	};
}

/* A NOTE ON PARTY ISSUES
 * It's worth noting that party_issues is a special type of issue, of similar structure but used in a different
 * way. Party issues can never have an "allow" condition and are always unordered. Even if a mod was to specify
 * those clauses, OV2's behaviour should be to simply disregard them, as they are meaningless to the context of
 * party issues.
 * Conversely, every attempt to read the list of reform types should skip over "party_issues".
*/