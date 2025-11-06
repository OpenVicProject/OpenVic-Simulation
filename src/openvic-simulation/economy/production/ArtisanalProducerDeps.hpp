#pragma once

namespace OpenVic {
	struct EconomyDefines;
	struct ModifierEffectCache;

	struct ArtisanalProducerDeps {
		EconomyDefines const& economy_defines;
		ModifierEffectCache const& modifier_effect_cache;
	};
}