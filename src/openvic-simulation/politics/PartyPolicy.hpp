#pragma once

#include "openvic-simulation/politics/BaseIssue.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct PartyPolicy;

	// PartyPolicy group (i.e. trade_policy)
	struct PartyPolicyGroup : HasIndex<PartyPolicyGroup, party_policy_group_index_t>, BaseIssueGroup {
		friend struct IssueManager;

	public:
		PartyPolicyGroup(
			std::string_view new_identifier,
			index_t new_index
		) : HasIndex { new_index }, BaseIssueGroup { new_identifier } {}
		PartyPolicyGroup(PartyPolicyGroup&&) = default;

		std::span<PartyPolicy const* const> get_party_policies() const {
			return { reinterpret_cast<PartyPolicy const* const*>(get_issues().data()), get_issues().size() };
		}
	};

	// PartyPolicy (i.e. protectionism)
	struct PartyPolicy : HasIndex<PartyPolicy, party_policy_index_t>, BaseIssue {
	public:
		PartyPolicy(
			index_t new_index,
			std::string_view new_identifier,
			colour_t new_colour,
			ModifierValue&& new_values,
			PartyPolicyGroup const& new_issue_group,
			RuleSet&& new_rules,
			bool new_is_jingoism
		) : HasIndex { new_index },
			BaseIssue {
				new_identifier,
				new_colour,
				std::move(new_values),
				new_issue_group,
				std::move(new_rules),
				new_is_jingoism,
				modifier_type_t::PARTY_POLICY
			} {}
		PartyPolicy(PartyPolicy&&) = default;
	};
}