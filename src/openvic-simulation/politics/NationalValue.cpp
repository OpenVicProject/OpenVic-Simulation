#include "NationalValue.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

NationalValue::NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers)
	: Modifier { new_identifier, std::move(new_modifiers) } {}

bool NationalValueManager::add_national_value(std::string_view identifier, ModifierValue&& modifiers) {
	if (identifier.empty()) {
		Logger::error("Invalid national value identifier - empty!");
		return false;
	}

	return national_values.add_item({ identifier, std::move(modifiers) });
}

bool NationalValueManager::load_national_values_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		national_values,
		[this, &modifier_manager](std::string_view national_value_identifier, ast::NodeCPtr value) -> bool {
			ModifierValue modifiers;

			bool ret = modifier_manager.expect_modifier_value(move_variable_callback(modifiers))(value);

			ret &= add_national_value(national_value_identifier, std::move(modifiers));
			return ret;
		}
	)(root);

	lock_national_values();

	return ret;
}
