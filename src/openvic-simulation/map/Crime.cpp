#include "Crime.hpp"

using namespace OpenVic;

Crime::Crime(
	index_t new_index,
	std::string_view new_identifier,
	ModifierValue&& new_values,
	icon_t new_icon,
	ConditionScript&& new_trigger,
	bool new_default_active
) : HasIndex { new_index },
	TriggeredModifier { new_identifier, std::move(new_values), modifier_type_t::CRIME, new_icon, std::move(new_trigger) },
	is_default_active { new_default_active } {}
