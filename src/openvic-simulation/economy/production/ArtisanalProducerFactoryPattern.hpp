#pragma once

#include <cstdint>

#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct ArtisanalProducerFactoryPattern {
	private:
		int32_t index = -1;
		memory::vector<ProductionType const*> unlocked_artisanal_production_types;
		GoodInstanceManager const& good_instance_manager;
		ModifierEffectCache const& modifier_effect_cache;
		ProductionTypeManager const& production_type_manager;

		void recalculate_unlocked_artisanal_production_types();

	public:
		ArtisanalProducerFactoryPattern(
			GoodInstanceManager const& new_good_instance_manager,
			ModifierEffectCache const& new_modifier_effect_cache,
			ProductionTypeManager const& new_production_type_manager
		);

		memory::unique_ptr<ArtisanalProducer> CreateNewArtisanalProducer();
	};
}