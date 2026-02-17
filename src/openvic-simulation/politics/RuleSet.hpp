#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/core/string/Utility.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/OptionalBool.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	//TODO replace with strategy pattern?
	enum struct ApportionmentMethod : std::uint8_t {
		largest_share,
		dhont,
		sainte_laque
	};
	enum struct UpperHouseComposition : std::uint8_t {
		same_as_ruling_party,
		rich_only,
		state_vote,
		population_vote
	};
	enum struct CulturalVotingRight : std::uint8_t {
		primary_culture_voting,
		culture_voting,
		all_voting
	};

	struct RuleSet {
	private:
		struct RuleDefinition {
			const std::string_view name;
			OptionalBool const& value;

			template<string_literal Name>
			static constexpr RuleDefinition Define(OptionalBool const& value) {
				return RuleDefinition { Name, value };
			}
		};

		//in the order of Vic2 tooltip
		#define DO_FOR_ALL_RULES_SEP(F, SEP) \
			F(build_factory) SEP \
			F(expand_factory) SEP \
			F(open_factory) SEP \
			F(destroy_factory) SEP \
			F(build_railway) SEP \
			F(factory_priority) SEP \
			F(can_subsidise) SEP \
			F(pop_build_factory) SEP \
			F(pop_expand_factory) SEP \
			F(pop_open_factory) SEP \
			F(delete_factory_if_no_input) SEP \
			F(build_factory_invest) SEP \
			F(expand_factory_invest) SEP \
			F(open_factory_invest) SEP \
			F(build_railway_invest) SEP \
			F(can_invest_in_pop_projects) SEP \
			F(pop_build_factory_invest) SEP \
			F(pop_expand_factory_invest) SEP \
			F(pop_open_factory_invest) SEP \
			F(allow_foreign_investment) SEP \
			F(primary_culture_voting) SEP \
			F(culture_voting) SEP \
			F(all_voting) SEP \
			F(largest_share) SEP \
			F(dhont) SEP \
			F(sainte_laque) SEP \
			F(rich_only) SEP \
			F(same_as_ruling_party) SEP \
			F(state_vote) SEP \
			F(population_vote) SEP \
			F(slavery_allowed)

		#define RULE_FIELDS(RULE) \
			OptionalBool RULE : 2 = OptionalBool::UNSPECIFIED;
		
		DO_FOR_ALL_RULES_SEP(RULE_FIELDS,)
		#undef RULE_FIELDS

		template <typename EnumType, size_t N>
		EnumType select_with_priority(const RuleDefinition (&rules)[N]) const {
			const std::string_view context_name = type_name<EnumType>();

			// 1. Check for explicit TRUE flags (Highest Priority)
			for (size_t i = 0; i < N; ++i) {
				if (rules[i].value == OptionalBool::TRUE) {
					// Log conflicts with lower-priority methods
					for (size_t j = i + 1; j < N; ++j) {
						if (OV_unlikely(rules[j].value == OptionalBool::TRUE)) {
							spdlog::warn_s(
								"Both {} and {} are enabled for {}. {} is picked.", 
								rules[i].name,
								rules[j].name,
								context_name,
								rules[i].name
							);
						}
					}
					return EnumType(i);
				}
			}

			// 2. Check for UNSPECIFIED (Defaulting logic)
			for (size_t i = 0; i < N; ++i) {
				if (rules[i].value == OptionalBool::UNSPECIFIED) {
					spdlog::warn_s("No {} enabled. Picking {} (unspecified).", context_name, rules[i].name);
					return EnumType(i);
				}
			}

			// 3. Absolute Fallback
			spdlog::error_s("All {} options are disabled. Falling back to {}.", context_name, rules[0].name);
			return EnumType(0);
		}

	#define DEF(VALUE) RuleDefinition::Define<#VALUE>(VALUE)

	#define RESOLVE_DEFAULT_FALSE(METHOD_NAME, FIELD) \
	bool METHOD_NAME() const { \
		const bool is_true = FIELD == OptionalBool::TRUE; \
		if (FIELD == OptionalBool::UNSPECIFIED) { \
			spdlog::warn_s("{} is not specified, returning {}.", #FIELD, is_true); \
		} \
		return is_true; \
	}

	[[nodiscard]] OptionalBool merge_rule(OptionalBool current, OptionalBool other, std::string_view name) {
		// If they are the same or 'other' provides no new info, stay as we are.
		if (current == other || other == OptionalBool::UNSPECIFIED) {
			return current;
		}

		// If we were unspecified, take whatever 'other' has.
		if (current == OptionalBool::UNSPECIFIED) {
			return other;
		}

		// Conflict: One is TRUE and the other is FALSE.
		// Since 'other' isn't UNSPECIFIED and isn't the same as 'current',
		// and we know 'current' isn't UNSPECIFIED, they must be TRUE/FALSE opposites.
		spdlog::warn_s("{} is both false and true, true wins.", name);
		return OptionalBool::TRUE;
	}
	
	static memory::string make_rule_localisation_key(std::string_view identifier) {
		return "RULE_" + ascii_toupper(identifier);
	}

	public:
		#define MAP_FROM_BOOL(RULE) \
			#RULE, ZERO_OR_ONE, NodeTools::expect_bool( \
				[&ruleset](const bool value) mutable -> bool { \
					const bool ret = ruleset.RULE == OptionalBool::UNSPECIFIED; \
					ruleset.RULE = value ? OptionalBool::TRUE : OptionalBool::FALSE; \
					return ret; \
				} \
			)
		#define COMMA ,

		static NodeTools::node_callback_t expect_rule_set(NodeTools::callback_t<RuleSet&&> ruleset_callback) {
			return [ruleset_callback](ast::NodeCPtr root) mutable -> bool {
				RuleSet ruleset;
				using enum NodeTools::dictionary_entry_t::expected_count_t;
				bool outer_result = NodeTools::expect_dictionary_keys(
					DO_FOR_ALL_RULES_SEP(MAP_FROM_BOOL, COMMA)
				)(root);
				outer_result &= ruleset_callback(std::move(ruleset));
				return outer_result;
			};
		}

		#undef COMMA
		#undef MAP_FROM_BOOL

		#define ADD_IF_NOT_UNSPECIFIED(RULE) \
			if (RULE != OptionalBool::UNSPECIFIED) { \
				result.emplace(make_rule_localisation_key(#RULE), RULE == OptionalBool::TRUE); \
			}

		ordered_map<memory::string, bool> get_localisation_keys_and_values() const {
			ordered_map<memory::string, bool> result {};
			DO_FOR_ALL_RULES_SEP(ADD_IF_NOT_UNSPECIFIED,)
			return result;
		}
		#undef ADD_IF_NOT_UNSPECIFIED

	//political
		ApportionmentMethod get_apportionment_method() const {
			const RuleDefinition rules[] {
				DEF(largest_share),
				DEF(dhont),
				DEF(sainte_laque)
			};
			return select_with_priority<ApportionmentMethod>(rules);
		}
		UpperHouseComposition get_upper_house_composition() const {
			const RuleDefinition rules[] {
				DEF(same_as_ruling_party),
				DEF(rich_only),
				DEF(state_vote),
				DEF(population_vote)
			};
			return select_with_priority<UpperHouseComposition>(rules);
		}
		CulturalVotingRight get_cultural_voting_rights() const {
			const RuleDefinition rules[] {
				DEF(primary_culture_voting),
				DEF(culture_voting),
				DEF(all_voting)
			};
			return select_with_priority<CulturalVotingRight>(rules);
		}
  	#undef DEF
		RESOLVE_DEFAULT_FALSE(is_slavery_legal, slavery_allowed)
	//economic
		RESOLVE_DEFAULT_FALSE(may_build_infrastructure_domestically, build_railway)
		RESOLVE_DEFAULT_FALSE(may_build_factory_domestically, build_factory)
		RESOLVE_DEFAULT_FALSE(may_expand_factory_domestically, expand_factory)
		RESOLVE_DEFAULT_FALSE(may_open_factory_domestically, open_factory)
		bool may_close_factory_domestically() const {
			return may_open_factory_domestically();
		}
		RESOLVE_DEFAULT_FALSE(may_destroy_factory_domestically, destroy_factory)
		RESOLVE_DEFAULT_FALSE(may_subsidise_factory_domestically, can_subsidise)
		RESOLVE_DEFAULT_FALSE(may_set_factory_priority_domestically, factory_priority)

		RESOLVE_DEFAULT_FALSE(pop_may_build_factory_domestically, pop_build_factory)
		RESOLVE_DEFAULT_FALSE(pop_may_expand_factory_domestically, pop_expand_factory)
		RESOLVE_DEFAULT_FALSE(pop_may_open_factory_domestically, pop_open_factory)
		RESOLVE_DEFAULT_FALSE(may_automatically_delete_factory_if_no_input_domestically, delete_factory_if_no_input)
		RESOLVE_DEFAULT_FALSE(may_invest_in_pop_projects_domestically, can_invest_in_pop_projects)

		RESOLVE_DEFAULT_FALSE(may_invest_in_building_factory_abroad, build_factory_invest)
		RESOLVE_DEFAULT_FALSE(may_invest_in_expanding_factory_abroad, expand_factory_invest)
		RESOLVE_DEFAULT_FALSE(may_invest_in_opening_factory_abroad, open_factory_invest)
		RESOLVE_DEFAULT_FALSE(pop_may_invest_in_building_factory_abroad, pop_build_factory_invest)
		RESOLVE_DEFAULT_FALSE(pop_may_invest_in_expanding_factory_abroad, pop_expand_factory_invest)
		RESOLVE_DEFAULT_FALSE(pop_may_invest_in_opening_factory_abroad, pop_open_factory_invest)
		RESOLVE_DEFAULT_FALSE(may_invest_in_expanding_infrastructure_abroad, build_railway_invest)

		RESOLVE_DEFAULT_FALSE(foreigners_may_invest, allow_foreign_investment)
	#undef RESOLVE_DEFAULT_FALSE

	#define ASSIGN_MERGED_RULE(RULE) RULE = merge_rule(RULE, other.RULE, #RULE);
		void add_ruleset(RuleSet const& other) {
			DO_FOR_ALL_RULES_SEP(ASSIGN_MERGED_RULE,)
		}
	#undef ASSIGN_MERGED_RULE
	#undef DO_FOR_ALL_RULES
	};
}