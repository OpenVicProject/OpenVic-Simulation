#include "Rule.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Rule::Rule(std::string_view new_identifier, rule_group_t new_group, size_t new_index)
  : HasIdentifier { new_identifier }, group { new_group }, index { new_index } {}

RuleSet::RuleSet(rule_group_map_t&& new_rule_groups) : rule_groups { std::move(new_rule_groups) } {}

bool RuleSet::trim_and_resolve_conflicts(bool log) {
	bool ret = true;
	for (auto [group, rule_map] : mutable_iterator(rule_groups)) {
		if (Rule::is_mutually_exclusive_group(group)) {
			Rule const* primary_rule = nullptr;
			for (auto const& [rule, value] : rule_map) {
				if (value) {
					if (primary_rule == nullptr) {
						primary_rule = rule;
					} else {
						if (primary_rule->get_index() < rule->get_index()) {
							primary_rule = rule;
						}
						ret = false;
					}
				}
			}
			if (log) {
				for (auto const& [rule, value] : rule_map) {
					if (value) {
						if (rule != primary_rule) {
							Logger::error(
								"Conflicting mutually exclusive rule: ", rule, " superceeded by ", primary_rule, " - removing!"
							);
						}
					} else {
						Logger::warning("Disabled mutually exclusive rule: ", rule, " - removing!");
					}
				}
			}
			rule_map.clear();
			if (primary_rule != nullptr) {
				rule_map.emplace(primary_rule, true);
			}
		}
	}
	return ret;
}

size_t RuleSet::get_rule_group_count() const {
	return rule_groups.size();
}

size_t RuleSet::get_rule_count() const {
	size_t ret = 0;
	for (auto const& [group, rule_map] : rule_groups) {
		ret += rule_map.size();
	}
	return ret;
}

RuleSet::rule_map_t const& RuleSet::get_rule_group(Rule::rule_group_t group, bool* successful) const {
	const rule_group_map_t::const_iterator it = rule_groups.find(group);
	if (it != rule_groups.end()) {
		if (successful != nullptr) {
			*successful = true;
		}
		return it->second;
	}
	if (successful != nullptr) {
		*successful = false;
	}
	static const rule_map_t empty_map {};
	return empty_map;
}

bool RuleSet::get_rule(Rule const* rule, bool* successful) const {
	if (rule == nullptr) {
		Logger::error("Invalid rule - null!");
		return false;
	}
	rule_map_t const& rule_map = get_rule_group(rule->get_group());
	const rule_map_t::const_iterator it = rule_map.find(rule);
	if (it != rule_map.end()) {
		if (successful != nullptr) {
			*successful = true;
		}
		return it->second;
	}
	if (successful != nullptr) {
		*successful = false;
	}
	return Rule::is_default_enabled(rule->get_group());
}

bool RuleSet::has_rule(Rule const* rule) const {
	if (rule == nullptr) {
		Logger::error("Invalid rule - null!");
		return false;
	}
	return get_rule_group(rule->get_group()).contains(rule);
}

bool RuleSet::set_rule(Rule const* rule, bool value) {
	if (rule == nullptr) {
		Logger::error("Invalid rule - null!");
		return false;
	}
	rule_map_t& rule_map = rule_groups[rule->get_group()];
	return rule_groups[rule->get_group()].emplace(rule, value).second;
}

RuleSet& RuleSet::operator|=(RuleSet const& right) {
	for (auto const& [group, rule_map] : right.rule_groups) {
		for (auto const& [rule, value] : rule_map) {
			rule_groups[group][rule] |= value;
		}
	}
	return *this;
}

RuleSet RuleSet::operator|(RuleSet const& right) const {
	RuleSet ret = *this;
	return ret |= right;
}

bool RuleManager::add_rule(std::string_view identifier, Rule::rule_group_t group) {
	if (identifier.empty()) {
		Logger::error("Invalid rule identifier - empty!");
		return false;
	}
	return rules.add_item({ identifier, group, rule_group_sizes[group]++ });
}

bool RuleManager::setup_rules(BuildingTypeManager const& building_type_manager) {
	bool ret = true;

	using enum Rule::rule_group_t;

	static const ordered_map<Rule::rule_group_t, std::vector<std::string_view>> hardcoded_rules {
		{ ECONOMY, {
			"expand_factory", "open_factory", "destroy_factory", "pop_build_factory", "pop_expand_factory", "pop_open_factory",
			"can_subsidise", "factory_priority", "delete_factory_if_no_input", "build_factory_invest", "expand_factory_invest",
			"open_factory_invest", "build_railway_invest", "pop_build_factory_invest", "pop_expand_factory_invest",
			"can_invest_in_pop_projects", "allow_foreign_investment"
		} },
		{ CITIZENSHIP, { "primary_culture_voting", "culture_voting", "all_voting" } },
		{ SLAVERY, { "slavery_allowed" } },
		{ UPPER_HOUSE, { "same_as_ruling_party", "rich_only", "state_vote", "population_vote" } },
		{ VOTING, {
			"largest_share" /* First Past the Post */,
			"dhont" /* Jefferson Method */,
			"sainte_laque" /* Proportional Representation */
		} }
	};

	size_t rule_count = building_type_manager.get_building_type_types().size();
	for (auto const& [group, rule_list] : hardcoded_rules) {
		rule_count += rule_list.size();
	}
	rules.reserve(rule_count);

	for (auto const& [group, rule_list] : hardcoded_rules) {
		for (std::string_view const& rule : rule_list) {
			ret &= add_rule(rule, group);
		}
	}

	for (std::string const& type : building_type_manager.get_building_type_types()) {
		std::string build_rule_string = "build_";
		if (type != "infrastructure") {
			build_rule_string += type;
		} else {
			build_rule_string += "railway";
		}
		ret &= add_rule(build_rule_string, ECONOMY);
	}

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
							if (!ruleset.rule_groups[rule->get_group()].emplace(rule, value).second) {
								Logger::warning("Duplicate rule entry: ", rule, " - ignoring!");
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
		ret &= ruleset.trim_and_resolve_conflicts(true);
		ret &= ruleset_callback(std::move(ruleset));
		return ret;
	};
}

namespace OpenVic { // so the compiler shuts up (copied from Modifier.cpp)
	std::ostream& operator<<(std::ostream& stream, RuleSet const& value) {
		for (auto const& [group, rule_map] : value.rule_groups) {
			for (auto const& [rule, value] : rule_map) {
				stream << rule << ": " << (value ? "yes" : "no") << "\n";
			}
		}
		return stream;
	}
}
