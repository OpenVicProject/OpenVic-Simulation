#pragma once

#include <string_view>

#include "openvic-simulation/modifier/Modifier.hpp"

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
}
