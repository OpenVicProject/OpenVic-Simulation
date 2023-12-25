#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct RuleManager;

	struct Rule : HasIdentifier {
		friend struct RuleManager;

	private:
		Rule(std::string_view new_identifier);

	public:
		Rule(Rule&&) = default;
	};

	struct RuleSet {
		friend struct RuleManager;

		using rule_map_t = ordered_map<Rule const*, bool>;

	private:
		rule_map_t rules;

	public:
		RuleSet() = default;
		RuleSet(rule_map_t&& new_rules);
		RuleSet(RuleSet const&) = default;
		RuleSet(RuleSet&&) = default;

		RuleSet& operator=(RuleSet const&) = default;
		RuleSet& operator=(RuleSet&&) = default;

		size_t get_rule_count() const;

		bool get_rule(Rule const* rule, bool* successful = nullptr);
		bool has_rule(Rule const* rule) const;

		RuleSet& operator|=(RuleSet const& right);
		RuleSet operator|(RuleSet const& right) const;

		friend std::ostream& operator<<(std::ostream& stream, RuleSet const& value);
	};

	struct RuleManager {
	private:
		IdentifierRegistry<Rule> IDENTIFIER_REGISTRY(rule);

	public:
		bool add_rule(std::string_view identifier);

		bool setup_rules();

		NodeTools::node_callback_t expect_rule_set(NodeTools::callback_t<RuleSet&&> ruleset_callback) const;
	};
}
