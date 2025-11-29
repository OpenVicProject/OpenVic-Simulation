#pragma once

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"

namespace OpenVic {
	struct EconomyDefines;
	struct GoodDefinition;
	struct ModifierEffectCache;

	struct ArtisanalProducerDeps {
		EconomyDefines const& economy_defines;
		forwardable_span<const GoodDefinition> good_keys;
		ModifierEffectCache const& modifier_effect_cache;
	};
}