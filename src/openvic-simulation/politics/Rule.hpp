#pragma once

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct RuleManager;
	struct BuildingTypeManager;

	/* The index of the Rule within its group, used to determine precedence in mutually exclusive rule groups. */
	struct Rule : HasIdentifier, HasIndex<Rule> {
		enum class rule_group_t : uint8_t {
			ECONOMY, CITIZENSHIP, SLAVERY, UPPER_HOUSE, VOTING
		};

		static constexpr bool is_mutually_exclusive_group(rule_group_t group) {
			using enum rule_group_t;
			return group == CITIZENSHIP || group == UPPER_HOUSE || group == VOTING;
		}

		/* Mutually exclusive groups must be default disabled! */
		static constexpr bool is_default_enabled(rule_group_t group) {
			using enum rule_group_t;
			return !is_mutually_exclusive_group(group) && group != SLAVERY;
		}

	private:
		memory::string PROPERTY(localisation_key);

	public:
		const rule_group_t group;

		Rule(
			std::string_view new_identifier, rule_group_t new_group, index_t new_index, std::string_view new_localisation_key
		);
		Rule(Rule&&) = default;
	};

	struct RuleSet {
		friend struct RuleManager;

		using rule_map_t = ordered_map<Rule const*, bool>;
		using rule_group_map_t = ordered_map<Rule::rule_group_t, rule_map_t>;

	private:
		rule_group_map_t PROPERTY(rule_groups);

	public:
		RuleSet() {};
		RuleSet(rule_group_map_t&& new_rule_groups);
		RuleSet(RuleSet const&) = default;
		RuleSet(RuleSet&&) = default;

		RuleSet& operator=(RuleSet const&) = default;
		RuleSet& operator=(RuleSet&&) = default;

		/* Removes conflicting and disabled mutually exclusive rules. If log is true, a warning will be emitted for each
		 * removed disabled rule and an error will be emitted for each removed conflicting rule. Returns true if no conflicts
		 * are found (regardless of whether disabled rules are removed or not), false otherwise. */
		bool trim_and_resolve_conflicts(bool log);
		size_t get_rule_group_count() const;
		size_t get_rule_count() const;
		void clear();
		bool empty() const;

		rule_map_t const& get_rule_group(Rule::rule_group_t group, bool* rule_group_found = nullptr) const;
		bool get_rule(Rule const& rule, bool* rule_found = nullptr) const;
		bool has_rule(Rule const& rule) const;

		/* Sets the rule to the specified value. Returns false if there was an existing rule, regardless of its value. */
		bool set_rule(Rule const& rule, bool value);

		RuleSet& operator|=(RuleSet const& right);
		RuleSet operator|(RuleSet const& right) const;

		friend std::ostream& operator<<(std::ostream& stream, RuleSet const& value);
	};

	struct RuleManager {
	private:
		IdentifierRegistry<Rule> IDENTIFIER_REGISTRY(rule);
		ordered_map<Rule::rule_group_t, size_t> rule_group_sizes;

	public:
		bool add_rule(std::string_view identifier, Rule::rule_group_t group, std::string_view localisation_key = {});

		bool setup_rules(BuildingTypeManager const& building_type_manager);

		NodeTools::node_callback_t expect_rule_set(NodeTools::callback_t<RuleSet&&> ruleset_callback) const;
	};
}
