#include "openvic-simulation/DefinitionManager.hpp"

#include "openvic-simulation/history/HistoryManager.hpp"
#include "openvic-simulation/interface/UI.hpp"

using namespace OpenVic;

DefinitionManager::DefinitionManager() : ui_manager(*this), history_manager(*this) {}
