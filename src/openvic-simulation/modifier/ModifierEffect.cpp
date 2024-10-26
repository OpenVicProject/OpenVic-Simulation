#include "ModifierEffect.hpp"

#include "openvic-simulation/utility/StringUtils.hpp"

using namespace OpenVic;

std::string ModifierEffect::target_to_string(target_t target) {
	using enum target_t;

	if (target == NO_TARGETS) {
		return "NO TARGETS";
	}

	if (target == ALL_TARGETS) {
		return "ALL TARGETS";
	}

	static constexpr std::string_view SEPARATOR = " | ";

	std::string ret;

	const auto append_target = [target, &ret](target_t check_target, std::string_view target_str) -> void {
		if (!ModifierEffect::excludes_targets(target, check_target)) {
			if (!ret.empty()) {
				ret += SEPARATOR;
			}
			ret += target_str;
		}
	};

	append_target(COUNTRY, "COUNTRY");
	append_target(PROVINCE, "PROVINCE");
	append_target(UNIT, "UNIT");
	append_target(~ALL_TARGETS, "INVALID TARGET");

	return ret;
}

std::string ModifierEffect::make_default_modifier_effect_localisation_key(std::string_view identifier) {
	return "MODIFIER_" + StringUtils::string_toupper(identifier);
}

ModifierEffect::ModifierEffect(
	std::string_view new_identifier, bool new_is_positive_good, format_t new_format, target_t new_targets,
	std::string_view new_localisation_key, bool new_has_no_effect
) : HasIdentifier { new_identifier }, positive_good { new_is_positive_good }, format { new_format }, targets { new_targets },
	localisation_key {
		new_localisation_key.empty() ? make_default_modifier_effect_localisation_key(new_identifier) : new_localisation_key
	}, no_effect { new_has_no_effect } {}
