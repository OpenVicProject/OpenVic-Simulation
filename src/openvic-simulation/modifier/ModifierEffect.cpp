#include "ModifierEffect.hpp"

#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

std::string ModifierEffect::make_default_modifier_effect_localisation_key(std::string_view identifier) {
	return "MODIFIER_" + StringUtils::string_toupper(identifier);
}

ModifierEffect::ModifierEffect(
	std::string_view new_identifier, bool new_positive_good, format_t new_format, target_t new_targets,
	std::string_view new_localisation_key
) : HasIdentifier { new_identifier }, positive_good { new_positive_good }, format { new_format }, targets { new_targets },
	localisation_key {
		new_localisation_key.empty() ? make_default_modifier_effect_localisation_key(new_identifier) : new_localisation_key
	} {}
