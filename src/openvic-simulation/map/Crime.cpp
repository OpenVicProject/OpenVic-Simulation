#include "Crime.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Crime::Crime(
	index_t new_index,
	std::string_view new_identifier,
	ModifierValue&& new_values,
	icon_t new_icon,
	ConditionScript&& new_trigger,
	bool new_default_active
) : HasIndex { new_index },
	TriggeredModifier { new_identifier, std::move(new_values), modifier_type_t::CRIME, new_icon, std::move(new_trigger) },
	default_active { new_default_active } {}

bool CrimeManager::add_crime_modifier(
	std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger,
	bool default_active
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid crime modifier effect identifier - empty!");
		return false;
	}

	return crime_modifiers.emplace_item(
		identifier,
		duplicate_warning_callback,
		Crime::index_t { get_crime_modifier_count() }, identifier, std::move(values), icon, std::move(trigger), default_active
	);
}

bool CrimeManager::load_crime_modifiers(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		crime_modifiers,
		[this, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			using enum scope_type_t;

			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;
			ConditionScript trigger { PROVINCE, NO_SCOPE, NO_SCOPE };
			bool default_active = false;

			bool ret = NodeTools::expect_dictionary_keys_and_default(
				modifier_manager.expect_base_province_modifier(modifier_value),
				"icon", ZERO_OR_ONE, expect_uint(assign_variable_callback(icon)),
				"trigger", ZERO_OR_ONE, trigger.expect_script(),
				"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(default_active))
			)(value);

			ret &= add_crime_modifier(key, std::move(modifier_value), icon, std::move(trigger), default_active);

			return ret;
		}
	)(root);

	lock_crime_modifiers();

	return ret;
}

bool CrimeManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (Crime& crime : crime_modifiers.get_items()) {
		ret &= crime.parse_scripts(
			definition_manager,
			true // Script can be null, meaning this won't emit an error message if the crime has no trigger in its definition
		);
	}

	return ret;
}
