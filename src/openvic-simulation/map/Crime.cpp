#include "Crime.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Crime::Crime(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon, bool new_default_active)
  : TriggeredModifier { new_identifier, std::move(new_values), new_icon }, default_active { new_default_active } {}

bool CrimeManager::add_crime_modifier(
	std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon, bool default_active
) {
	if (identifier.empty()) {
		Logger::error("Invalid crime modifier effect identifier - empty!");
		return false;
	}
	return crime_modifiers.add_item({ identifier, std::move(values), icon, default_active }, duplicate_warning_callback);
}

bool CrimeManager::load_crime_modifiers(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		crime_modifiers,
		[this, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			Modifier::icon_t icon = 0;
			bool default_active = false;
			bool ret = modifier_manager.expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
				"icon", ZERO_OR_ONE, expect_uint(assign_variable_callback(icon)),
				"trigger", ONE_EXACTLY, success_callback, // TODO - load condition
				"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(default_active))
			)(value);
			ret &= add_crime_modifier(key, std::move(modifier_value), icon, default_active);
			return ret;
		}
	)(root);
	lock_crime_modifiers();
	return ret;
}
