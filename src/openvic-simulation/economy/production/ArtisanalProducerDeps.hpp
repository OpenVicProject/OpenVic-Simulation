#pragma once

#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct EconomyDefines;
	struct GoodDefinition;
	struct ModifierEffectCache;

	struct ArtisanalProducerDeps {
		EconomyDefines const& economy_defines;
		utility::forwardable_span<const GoodDefinition> good_keys;
		ModifierEffectCache const& modifier_effect_cache;
	};
}