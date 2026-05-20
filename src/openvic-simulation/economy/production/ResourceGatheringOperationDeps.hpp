#pragma once

#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct MarketInstance;
	struct ModifierEffectCache;

	struct ResourceGatheringOperationDeps {
		MarketInstance& market_instance;
		ModifierEffectCache const& modifier_effect_cache;
		pop_type_index_t pop_type_count;
	};
}