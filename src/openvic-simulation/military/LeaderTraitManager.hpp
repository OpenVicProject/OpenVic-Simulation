#pragma once

#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct MilitaryDefines;
	struct ModifierEffectCache;

	struct LeaderTraitManager {
	private:
		IdentifierRegistry<LeaderTrait> IDENTIFIER_REGISTRY(leader_trait);
		memory::vector<LeaderTrait const*> SPAN_PROPERTY(personality_traits);
		memory::vector<LeaderTrait const*> SPAN_PROPERTY(background_traits);

		// As well as their background and personality traits, leaders get this modifier scaled by their prestige
		Modifier PROPERTY(leader_prestige_modifier);

	public:
		LeaderTraitManager();

		bool setup_leader_prestige_modifier(
			ModifierEffectCache const& modifier_effect_cache, MilitaryDefines const& military_defines
		);

		bool add_leader_trait(std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers);

		bool load_leader_traits_file(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);
	};
}
