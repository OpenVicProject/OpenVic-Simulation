#pragma once

#include <memory>

#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"

namespace OpenVic {
	struct Pop;

	struct ArtisanalProducerFactoryPattern {
		private:
			int index;
			std::vector<ProductionType const*> unlocked_artisanal_production_types;
			MarketInstance& market_instance;
			ModifierEffectCache const& modifier_effect_cache;
			ProductionTypeManager const& production_type_manager;
		
		public:
			ArtisanalProducerFactoryPattern(
				MarketInstance& new_market_instance,
				ModifierEffectCache const& new_modifier_effect_cache,
				ProductionTypeManager const& new_production_type_manager
			);

			std::unique_ptr<ArtisanalProducer> CreateNewArtisanalProducer();
	};
}