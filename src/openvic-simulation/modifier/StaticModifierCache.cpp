#include "StaticModifierCache.hpp"

#include "openvic-simulation/modifier/Modifier.hpp"

using namespace OpenVic;

StaticModifierCache::StaticModifierCache()
  : // Country modifiers
	very_easy_player { nullptr },
	easy_player { nullptr },
	hard_player { nullptr },
	very_hard_player { nullptr },
	very_easy_ai { nullptr },
	easy_ai { nullptr },
	hard_ai { nullptr },
	very_hard_ai { nullptr },
	base_modifier { nullptr },
	war { nullptr },
	peace { nullptr },
	disarming { nullptr },
	war_exhaustion { nullptr },
	infamy { nullptr },
	debt_default_to { nullptr },
	bad_debter { nullptr },
	great_power { nullptr },
	secondary_power { nullptr },
	civilised { nullptr },
	uncivilised { nullptr },
	literacy { nullptr },
	plurality { nullptr },
	generalised_debt_default { nullptr },
	total_occupation { nullptr },
	total_blockaded { nullptr },
	in_bankruptcy { nullptr },
	// Province modifiers
	overseas { nullptr },
	coastal { nullptr },
	non_coastal { nullptr },
	coastal_sea { nullptr },
	sea_zone { nullptr },
	land_province { nullptr },
	blockaded { nullptr },
	no_adjacent_controlled { nullptr },
	core { nullptr },
	has_siege { nullptr },
	occupied { nullptr },
	nationalism { nullptr },
	infrastructure { nullptr } {}

bool StaticModifierCache::load_static_modifiers(ModifierManager& modifier_manager) {
	bool ret = true;

	const auto set_static_modifier = [&modifier_manager, &ret](
		Modifier const*& modifier, std::string_view name, fixed_point_t multiplier = 1
	) -> void {
		Modifier* mutable_modifier = modifier_manager.static_modifiers.get_item_by_identifier(name);

		if (mutable_modifier != nullptr) {
			if (multiplier != fixed_point_t::_1()) {
				*mutable_modifier *= multiplier;
			}

			modifier = mutable_modifier;
		} else {
			Logger::error("Failed to set static modifier: ", name);
			ret = false;
		}
	};

	// Country modifiers
	set_static_modifier(very_easy_player, "very_easy_player");
	set_static_modifier(easy_player, "easy_player");
	set_static_modifier(hard_player, "hard_player");
	set_static_modifier(very_hard_player, "very_hard_player");
	set_static_modifier(very_easy_ai, "very_easy_ai");
	set_static_modifier(easy_ai, "easy_ai");
	set_static_modifier(hard_ai, "hard_ai");
	set_static_modifier(very_hard_ai, "very_hard_ai");

	set_static_modifier(base_modifier, "base_values");
	set_static_modifier(war, "war");
	set_static_modifier(peace, "peace");
	set_static_modifier(disarming, "disarming");
	set_static_modifier(war_exhaustion, "war_exhaustion");
	set_static_modifier(infamy, "badboy");
	set_static_modifier(debt_default_to, "debt_default_to");
	set_static_modifier(bad_debter, "bad_debter");
	set_static_modifier(great_power, "great_power");
	set_static_modifier(secondary_power, "second_power");
	set_static_modifier(civilised, "civ_nation");
	set_static_modifier(uncivilised, "unciv_nation");
	set_static_modifier(literacy, "average_literacy");
	set_static_modifier(plurality, "plurality");
	set_static_modifier(generalised_debt_default, "generalised_debt_default");
	set_static_modifier(total_occupation, "total_occupation", 100);
	set_static_modifier(total_blockaded, "total_blockaded");
	set_static_modifier(in_bankruptcy, "in_bankrupcy");

	// Province modifiers
	set_static_modifier(overseas, "overseas");
	set_static_modifier(coastal, "coastal");
	set_static_modifier(non_coastal, "non_coastal");
	set_static_modifier(coastal_sea, "coastal_sea");
	set_static_modifier(sea_zone, "sea_zone");
	set_static_modifier(land_province, "land_province");
	set_static_modifier(blockaded, "blockaded");
	set_static_modifier(no_adjacent_controlled, "no_adjacent_controlled");
	set_static_modifier(core, "core");
	set_static_modifier(has_siege, "has_siege");
	set_static_modifier(occupied, "occupied");
	set_static_modifier(nationalism, "nationalism");
	set_static_modifier(infrastructure, "infrastructure");

	return ret;
}
