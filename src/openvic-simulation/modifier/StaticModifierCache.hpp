#pragma once

#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct Modifier;
	struct ModifierManager;

	struct StaticModifierCache {
		friend struct ModifierManager;

	private:
		// Country modifiers
		Modifier const* PROPERTY(very_easy_player);
		Modifier const* PROPERTY(easy_player);
		Modifier const* PROPERTY(hard_player);
		Modifier const* PROPERTY(very_hard_player);
		Modifier const* PROPERTY(very_easy_ai);
		Modifier const* PROPERTY(easy_ai);
		Modifier const* PROPERTY(hard_ai);
		Modifier const* PROPERTY(very_hard_ai);

		Modifier const* PROPERTY(base_modifier);
		Modifier const* PROPERTY(war);
		Modifier const* PROPERTY(peace);
		Modifier const* PROPERTY(disarming);
		Modifier const* PROPERTY(war_exhaustion);
		Modifier const* PROPERTY(infamy);
		Modifier const* PROPERTY(debt_default_to);
		Modifier const* PROPERTY(bad_debter);
		Modifier const* PROPERTY(great_power);
		Modifier const* PROPERTY(secondary_power);
		Modifier const* PROPERTY(civilised);
		Modifier const* PROPERTY(uncivilised);
		Modifier const* PROPERTY(literacy);
		Modifier const* PROPERTY(plurality);
		Modifier const* PROPERTY(generalised_debt_default);
		Modifier const* PROPERTY(total_occupation);
		Modifier const* PROPERTY(total_blockaded);
		Modifier const* PROPERTY(in_bankruptcy);

		// Province modifiers
		Modifier const* PROPERTY(overseas);
		Modifier const* PROPERTY(coastal);
		Modifier const* PROPERTY(non_coastal);
		Modifier const* PROPERTY(coastal_sea);
		Modifier const* PROPERTY(sea_zone);
		Modifier const* PROPERTY(land_province);
		Modifier const* PROPERTY(blockaded);
		Modifier const* PROPERTY(no_adjacent_controlled);
		Modifier const* PROPERTY(core);
		Modifier const* PROPERTY(has_siege);
		Modifier const* PROPERTY(occupied);
		Modifier const* PROPERTY(nationalism);
		Modifier const* PROPERTY(infrastructure);

		StaticModifierCache();

		bool load_static_modifiers(ModifierManager& registry);

	public:
		StaticModifierCache(StaticModifierCache&&) = default;
	};
}
