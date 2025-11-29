#pragma once

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"

namespace OpenVic {
	struct MarketInstance;
	struct ModifierEffectCache;
	struct PopType;

	struct ResourceGatheringOperationDeps {
		MarketInstance& market_instance;
		ModifierEffectCache const& modifier_effect_cache;
		forwardable_span<const PopType> pop_type_keys;
	};
}