#include "HistoryMap.hpp"

#include "openvic-simulation/DefinitionManager.hpp"

using namespace OpenVic;

HistoryEntry::HistoryEntry(Date new_date) : date { new_date } {}

Date _HistoryMapHelperFuncs::_get_start_date(DefinitionManager const& definition_manager) {
	return definition_manager.get_define_manager().get_start_date();
}

Date _HistoryMapHelperFuncs::_get_end_date(DefinitionManager const& definition_manager) {
	return definition_manager.get_define_manager().get_end_date();
}
