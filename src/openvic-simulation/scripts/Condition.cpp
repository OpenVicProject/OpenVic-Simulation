#include "Condition.hpp"

using namespace OpenVic;

using enum value_type_t;
using enum scope_type_t;
using enum identifier_type_t;

Condition::Condition(
	std::string_view new_identifier, value_type_t new_value_type, scope_type_t new_scope,
	scope_type_t new_scope_change, identifier_type_t new_key_identifier_type,
	identifier_type_t new_value_identifier_type
) : HasIdentifier { new_identifier }, value_type { new_value_type }, scope { new_scope },
	scope_change { new_scope_change }, key_identifier_type { new_key_identifier_type },
	value_identifier_type { new_value_identifier_type } {}

ConditionNode::ConditionNode(
	Condition const* new_condition, value_t&& new_value, bool new_valid,
	HasIdentifier const* new_condition_key_item,
	HasIdentifier const* new_condition_value_item
) : condition { new_condition }, value { std::move(new_value) }, valid { new_valid },
	condition_key_item { new_condition_key_item }, condition_value_item { new_condition_key_item } {}
