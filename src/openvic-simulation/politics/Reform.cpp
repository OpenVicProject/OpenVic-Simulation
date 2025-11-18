#include "Reform.hpp"

using namespace OpenVic;

ReformType::ReformType(std::string_view new_identifier, bool new_is_uncivilised)
	: HasIdentifier { new_identifier }, is_uncivilised { new_is_uncivilised } {}

ReformGroup::ReformGroup(
	std::string_view new_identifier,
	index_t new_index,
	ReformType const& new_reform_type,
	bool new_is_ordered,
	bool new_is_administrative
) : BaseIssueGroup { new_identifier },
	HasIndex { new_index },
	reform_type { new_reform_type },
	is_ordered { new_is_ordered },
	is_administrative { new_is_administrative } {}

Reform::Reform(
	index_t new_index, std::string_view new_identifier,
	colour_t new_colour, ModifierValue&& new_values, ReformGroup const& new_reform_group,
	size_t new_ordinal, fixed_point_t new_administrative_multiplier, RuleSet&& new_rules, tech_cost_t new_technology_cost,
	ConditionScript&& new_allow, ConditionScript&& new_on_execute_trigger, EffectScript&& new_on_execute_effect
) : BaseIssue {
		new_identifier, new_colour, std::move(new_values), new_reform_group, std::move(new_rules), false,
		modifier_type_t::REFORM
	},
	HasIndex { new_index },
	ordinal { new_ordinal },
	administrative_multiplier { new_administrative_multiplier },
	technology_cost { new_technology_cost },
	allow { std::move(new_allow) },
	on_execute_trigger { std::move(new_on_execute_trigger) },
	on_execute_effect { std::move(new_on_execute_effect) } {}

bool Reform::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	ret &= allow.parse_script(true, definition_manager);
	ret &= on_execute_trigger.parse_script(true, definition_manager);
	ret &= on_execute_effect.parse_script(true, definition_manager);

	return ret;
}