#pragma once

#include <string_view>

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct LeaderTraitManager;

	struct LeaderTrait : Modifier {
		friend struct LeaderTraitManager;

		enum class trait_type_t { PERSONALITY, BACKGROUND };

	private:
		const trait_type_t PROPERTY(trait_type);

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

	public:
		LeaderTrait(LeaderTrait&&) = default;

		bool is_personality_trait() const;
		bool is_background_trait() const;
	};

	struct LeaderTraitManager {
	private:
		IdentifierRegistry<LeaderTrait> IDENTIFIER_REGISTRY(leader_trait);

	public:
		bool add_leader_trait(std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers);

		bool load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
