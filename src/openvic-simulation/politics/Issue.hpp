#pragma once

#include <cstddef>
#include <string_view>
#include "types/IdentifierRegistry.hpp"

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
		const bool ordered; //next_step_only, TODO default to false

		IssueGroup(const std::string_view new_identifier, IssueType const& new_type, bool ordered);
	
	public:
		IssueGroup(IssueGroup&&) = default;
		IssueType const& get_type() const;
		bool is_ordered() const;
	};

	//Issue type (i.e. yes_slavery)
	struct Issue : HasIdentifier {
		friend struct IssueManager;

	private:
		IssueGroup const& group;
		const size_t ordinal; //assigned by the parser to allow policy sorting

		//TODO - conditions to allow, policy modifiers, policy rule changes

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

	public:
		IssueManager();
		
		bool add_issue_type(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(IssueType, issue_type)

		bool add_issue_group(const std::string_view identifier, IssueType const* type, bool ordered);
		IDENTIFIER_REGISTRY_ACCESSORS(IssueGroup, issue_group)

		bool add_issue(const std::string_view identifier, IssueGroup const* group, size_t ordinal);
		IDENTIFIER_REGISTRY_ACCESSORS(Issue, issue)

		//TODO: bool load_issues_file(ast::NodeCPtr root);
	};
}

/* Structure is as follows:
 *  issue_type { (i.e. political_issues)
 *  	issue_group{ (i.e. slavery)
 *  		issue { (i.e. yes_slavery)
 *  			...
 *  		}
 *  	}
 *  }
 * NOTE ON PARTY ISSUES
 * Worth noting that party_issues is a special type of issue, of similar structure but used in a different way.
 * Party issues can never have an "allow" condition and they are always unordered. Even if a mod decides to add
 * them, OV2's behaviour should probably be to disregard them, as they are meaningless within the context.
 * Conversely, lists of available reforms should make it a point to ignore the "party_issues" family.
*/