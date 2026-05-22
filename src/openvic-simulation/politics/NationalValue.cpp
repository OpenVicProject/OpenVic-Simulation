#include "NationalValue.hpp"

using namespace OpenVic;

NationalValue::NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers)
	: Modifier { new_identifier, std::move(new_modifiers), modifier_type_t::NATIONAL_VALUE } {}
