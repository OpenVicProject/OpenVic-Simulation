#pragma once

#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct LeaderTrait : Modifier {

		enum class trait_type_t { PERSONALITY, BACKGROUND };

	public:
		const trait_type_t trait_type;

		/*
		 * Allowed modifiers for leaders:
		 * attack - integer
		 * defence - integer
		 * morale - %
		 * organisation - %
		 * reconnaissance - %
		 * speed - %
		 * attrition - %, negative is good
		 * experience - %
		 * reliability - decimal, mil gain or loss for associated POPs
		 */

		LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers);
		LeaderTrait(LeaderTrait&&) = default;

		bool is_personality_trait() const;
		bool is_background_trait() const;
	};

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

		bool load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
