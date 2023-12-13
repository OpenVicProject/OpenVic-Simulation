#include "HistoryMap.hpp"

#include "openvic-simulation/GameManager.hpp"

using namespace OpenVic;

HistoryEntry::HistoryEntry(Date new_date) : date { new_date } {}

Date _HistoryMapHelperFuncs::_get_start_date(GameManager const& game_manager) {
	return game_manager.get_define_manager().get_start_date();
}

Date _HistoryMapHelperFuncs::_get_end_date(GameManager const& game_manager) {
	return game_manager.get_define_manager().get_end_date();
}
