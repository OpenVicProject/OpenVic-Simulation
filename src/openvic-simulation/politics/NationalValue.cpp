#include "NationalValue.hpp"

#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

NationalValue::NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers)
	: Modifier { new_identifier, std::move(new_modifiers), modifier_type_t::NATIONAL_VALUE } {}

bool NationalValueManager::add_national_value(std::string_view identifier, ModifierValue&& modifiers) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid national value identifier - empty!");
		return false;
	}

	return national_values.emplace_item(
		identifier,
		identifier, std::move(modifiers)
	);
}

bool NationalValueManager::load_national_values_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	spdlog::scope scope { "common/nationalvalues.txt" };
	bool ret = expect_dictionary_reserve_length(
		national_values,
		[this, &modifier_manager](std::string_view national_value_identifier, ast::NodeCPtr value) -> bool {
			spdlog::scope scope { fmt::format("national value {}", national_value_identifier) };
			ModifierValue modifiers;
			bool ret = NodeTools::expect_dictionary(
				modifier_manager.expect_base_country_modifier(modifiers)
			)(value);

			ret &= add_national_value(national_value_identifier, std::move(modifiers));

			return ret;
		}
	)(root);

	lock_national_values();

	return ret;
}
