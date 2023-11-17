#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct LeaderTraitManager;

	struct LeaderTrait : HasIdentifier {
		friend struct LeaderTraitManager;

		enum class trait_type_t { PERSONALITY, BACKGROUND };

	private:
		const trait_type_t type;
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
		const ModifierValue modifiers;

		LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers);

	public:
		LeaderTrait(LeaderTrait&&) = default;

		trait_type_t get_trait_type() const;
		bool is_personality_trait() const;
		bool is_background_trait() const;
		ModifierValue const& get_modifiers() const;
	};

	struct LeaderTraitManager {
	private:
		IdentifierRegistry<LeaderTrait> leader_traits;
		static inline const string_set_t allowed_modifiers = {
			"attack", "defence", "morale", "organisation", "reconnaissance", "speed", "attrition", "experience", "reliability"
		};

	public:
		LeaderTraitManager();

		bool add_leader_trait(std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers);
		IDENTIFIER_REGISTRY_ACCESSORS(leader_trait)

		bool load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
