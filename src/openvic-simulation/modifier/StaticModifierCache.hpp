#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct StaticModifierCache {
		friend struct ModifierManager;

	private:
		// Country modifiers
		Modifier PROPERTY(very_easy_player);
		Modifier PROPERTY(easy_player);
		Modifier PROPERTY(hard_player);
		Modifier PROPERTY(very_hard_player);
		Modifier PROPERTY(very_easy_ai);
		Modifier PROPERTY(easy_ai);
		Modifier PROPERTY(hard_ai);
		Modifier PROPERTY(very_hard_ai);

		Modifier PROPERTY(base_modifier);
		Modifier PROPERTY(war);
		Modifier PROPERTY(peace);
		Modifier PROPERTY(disarming);
		Modifier PROPERTY(war_exhaustion);
		Modifier PROPERTY(infamy);
		Modifier PROPERTY(debt_default_to);
		Modifier PROPERTY(great_power);
		Modifier PROPERTY(secondary_power);
		Modifier PROPERTY(civilised);
		Modifier PROPERTY(uncivilised);
		Modifier PROPERTY(literacy);
		Modifier PROPERTY(plurality);
		Modifier PROPERTY(total_occupation);
		Modifier PROPERTY(total_blockaded);

		// Country event modifiers
		Modifier const* PROPERTY(in_bankruptcy);
		Modifier const* PROPERTY(bad_debtor);
		Modifier const* PROPERTY(generalised_debt_default); //possibly, as it's used to trigger gunboat CB

		// Province modifiers
		Modifier PROPERTY(overseas);
		Modifier PROPERTY(coastal);
		Modifier PROPERTY(non_coastal);
		Modifier PROPERTY(coastal_sea);
		Modifier PROPERTY(sea_zone);
		Modifier PROPERTY(land_province);
		Modifier PROPERTY(blockaded);
		Modifier PROPERTY(no_adjacent_controlled);
		Modifier PROPERTY(core);
		Modifier PROPERTY(has_siege);
		Modifier PROPERTY(occupied);
		Modifier PROPERTY(nationalism);
		Modifier PROPERTY(infrastructure);

		StaticModifierCache();

		bool load_static_modifiers(ModifierManager& modifier_manager, const ast::NodeCPtr root);

	public:
		StaticModifierCache(StaticModifierCache&&) = default;
	};
}
