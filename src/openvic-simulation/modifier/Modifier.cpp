#include "Modifier.hpp"

using namespace OpenVic;

std::string_view Modifier::modifier_type_to_string(modifier_type_t type) {
	using enum Modifier::modifier_type_t;

	switch (type) {
#define _CASE(X) case X: return #X;
	_CASE(EVENT)
	_CASE(STATIC)
	_CASE(TRIGGERED)
	_CASE(CRIME)
	_CASE(TERRAIN)
	_CASE(CLIMATE)
	_CASE(CONTINENT)
	_CASE(BUILDING)
	_CASE(LEADER)
	_CASE(UNIT_TERRAIN)
	_CASE(NATIONAL_VALUE)
	_CASE(NATIONAL_FOCUS)
	_CASE(PARTY_POLICY)
	_CASE(REFORM)
	_CASE(TECHNOLOGY)
	_CASE(INVENTION)
	_CASE(TECH_SCHOOL)
#undef _CASE
	default: return "INVALID MODIFIER TYPE";
	}
}

Modifier::Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type)
	: HasIdentifier { new_identifier }, ModifierValue { std::move(new_values) }, type { new_type } {}

IconModifier::IconModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon
) : Modifier { new_identifier, std::move(new_values), new_type }, icon { new_icon } {}

TriggeredModifier::TriggeredModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
	ConditionScript&& new_trigger
) : IconModifier { new_identifier, std::move(new_values), new_type, new_icon }, trigger { std::move(new_trigger) } {}

bool TriggeredModifier::parse_scripts(DefinitionManager const& definition_manager, bool can_be_null) {
	return trigger.parse_script(can_be_null, definition_manager);
}

ModifierInstance::ModifierInstance(Modifier const& new_modifier, Date new_expiry_date)
	: modifier { &new_modifier }, expiry_date { new_expiry_date } {}
