#include "Rule.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Rule::Rule(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

RuleSet::RuleSet(rule_map_t&& new_rules) : rules { std::move(new_rules) } {}

size_t RuleSet::get_rule_count() const {
	return rules.size();
}

bool RuleSet::get_rule(Rule const* rule, bool* successful) {
	const rule_map_t::const_iterator it = rules.find(rule);
	if (it != rules.end()) {
		if (successful != nullptr) {
			*successful = true;
		}
		return it->second;
	}
	if (successful != nullptr) {
		*successful = false;
	}
	return false;
}

bool RuleSet::has_rule(Rule const* rule) const {
	return rules.contains(rule);
}

RuleSet& RuleSet::operator|=(RuleSet const& right) {
	for (rule_map_t::value_type const& value : right.rules) {
		rules[value.first] |= value.second;
	}
	return *this;
}

RuleSet RuleSet::operator|(RuleSet const& right) const {
	RuleSet ret = *this;
	return ret |= right;
}

bool RuleManager::add_rule(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid rule identifier - empty!");
		return false;
	}
	return rules.add_item({ identifier });
}

bool RuleManager::setup_rules() {
	bool ret = true;

	/* Economic Rules */
	ret &= add_rule("build_factory");
	ret &= add_rule("expand_factory");
	ret &= add_rule("open_factory");
	ret &= add_rule("destroy_factory");

	ret &= add_rule("pop_build_factory");
	ret &= add_rule("pop_expand_factory");
	ret &= add_rule("pop_open_factory");

	ret &= add_rule("build_railway");
	ret &= add_rule("can_subsidise");
	ret &= add_rule("factory_priority");
	ret &= add_rule("delete_factory_if_no_input");

	ret &= add_rule("build_factory_invest");
	ret &= add_rule("expand_factory_invest");
	ret &= add_rule("open_factory_invest");
	ret &= add_rule("build_railway_invest");

	ret &= add_rule("pop_build_factory_invest");
	ret &= add_rule("pop_expand_factory_invest");

	ret &= add_rule("can_invest_in_pop_projects");
	ret &= add_rule("allow_foreign_investment");

	/* Citizenship Rules */
	ret &= add_rule("primary_culture_voting");
	ret &= add_rule("culture_voting");
	ret &= add_rule("all_voting");

	/* Slavery Rule */
	ret &= add_rule("slavery_allowed");

	/* Upper House Composition Rules */
	ret &= add_rule("same_as_ruling_party");
	ret &= add_rule("rich_only");
	ret &= add_rule("state_vote");
	ret &= add_rule("population_vote");

	/* Voting System Rules */
	ret &= add_rule("largest_share"); // First Past the Post
	ret &= add_rule("dhont"); // Jefferson Method
	ret &= add_rule("sainte_laque"); // Proportional Representation

	lock_rules();

	return ret;
}

node_callback_t RuleManager::expect_rule_set(callback_t<RuleSet&&> ruleset_callback) const {
	return [this, ruleset_callback](ast::NodeCPtr root) -> bool {
		RuleSet ruleset;
		bool ret = expect_dictionary(
			[this, &ruleset](std::string_view rule_key, ast::NodeCPtr rule_value) -> bool {
				Rule const* rule = get_rule_by_identifier(rule_key);
				if (rule != nullptr) {
					return expect_bool(
						[&ruleset, rule](bool value) -> bool {
							if (!ruleset.rules.emplace(rule, value).second) {
								Logger::warning("Duplicate rule entry: ", rule, " - overwriting existing value!");
							}
							return true;
						}
					)(rule_value);
				} else {
					Logger::error("Invalid rule identifier: ", rule_key);
					return false;
				}
			}
		)(root);
		ret &= ruleset_callback(std::move(ruleset));
		return ret;
	};
}

namespace OpenVic { // so the compiler shuts up (copied from Modifier.cpp)
	std::ostream& operator<<(std::ostream& stream, RuleSet const& value) {
		for (RuleSet::rule_map_t::value_type const& rule : value.rules) {
			stream << rule.first << ": " << (rule.second ? "yes" : "no") << "\n";
		}
		return stream;
	}
}
