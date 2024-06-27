#pragma once

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/diplomacy/DiplomaticAction.hpp"
#include "openvic-simulation/economy/EconomyManager.hpp"
#include "openvic-simulation/history/HistoryManager.hpp"
#include "openvic-simulation/interface/UI.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/Mapmode.hpp"
#include "openvic-simulation/military/MilitaryManager.hpp"
#include "openvic-simulation/misc/Decision.hpp"
#include "openvic-simulation/misc/Define.hpp"
#include "openvic-simulation/misc/Event.hpp"
#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/research/ResearchManager.hpp"
#include "openvic-simulation/scripts/ScriptManager.hpp"

namespace OpenVic {
	struct DefinitionManager {
	private:
		DefineManager PROPERTY_REF(define_manager);
		EconomyManager PROPERTY_REF(economy_manager);
		MilitaryManager PROPERTY_REF(military_manager);
		ModifierManager PROPERTY_REF(modifier_manager);
		PoliticsManager PROPERTY_REF(politics_manager);
		HistoryManager PROPERTY_REF(history_manager);
		ResearchManager PROPERTY_REF(research_manager);
		PopManager PROPERTY_REF(pop_manager);
		CountryDefinitionManager PROPERTY_REF(country_definition_manager);
		CrimeManager PROPERTY_REF(crime_manager);
		EventManager PROPERTY_REF(event_manager);
		DecisionManager PROPERTY_REF(decision_manager);
		UIManager PROPERTY_REF(ui_manager);
		DiplomaticActionManager PROPERTY_REF(diplomatic_action_manager);
		MapDefinition PROPERTY_REF(map_definition);
		MapmodeManager PROPERTY_REF(mapmode_manager);
		ScriptManager PROPERTY_REF(script_manager);
	};
}
