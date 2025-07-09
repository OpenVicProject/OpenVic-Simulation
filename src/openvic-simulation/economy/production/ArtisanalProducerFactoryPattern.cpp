#include "ArtisanalProducerFactoryPattern.hpp"

#include "openvic-simulation/economy/GoodInstance.hpp"

using namespace OpenVic;

ArtisanalProducerFactoryPattern::ArtisanalProducerFactoryPattern(
	GoodInstanceManager const& new_good_instance_manager, ModifierEffectCache const& new_modifier_effect_cache,
	ProductionTypeManager const& new_production_type_manager
)
	: good_instance_manager { new_good_instance_manager }, modifier_effect_cache { new_modifier_effect_cache },
	  production_type_manager { new_production_type_manager } {}

memory::unique_ptr<ArtisanalProducer> ArtisanalProducerFactoryPattern::CreateNewArtisanalProducer() {
	//TODO update unlocked_artisanal_production_types when goods are unlocked
	if (index == -1) {
		recalculate_unlocked_artisanal_production_types();
	}

	if (OV_unlikely(unlocked_artisanal_production_types.size() == 0)) {
		Logger::error("CreateNewArtisanalProducer was called but there are no artisanal production types.");
		return nullptr;
	}

	//TODO select production type the way Victoria 2 does it (random?)
	index = (index+1) % unlocked_artisanal_production_types.size();
	ProductionType const* random_artisanal_production_type = unlocked_artisanal_production_types[index];

	return memory::make_unique<ArtisanalProducer>(
		modifier_effect_cache,
		GoodDefinition::good_definition_map_t{},
		*random_artisanal_production_type,
		0
	);
}

void ArtisanalProducerFactoryPattern::recalculate_unlocked_artisanal_production_types() {
	unlocked_artisanal_production_types.clear();
	for (ProductionType const& production_type : production_type_manager.get_production_types()) {
		if (production_type.get_template_type() == ProductionType::template_type_t::ARTISAN) {
			GoodInstance const& good_instance = good_instance_manager.get_good_instance_from_definition(
				production_type.get_output_good()
			);
			if (!good_instance.get_is_available()) {
				continue;
			}

			unlocked_artisanal_production_types.push_back(&production_type);
		}
	}
}