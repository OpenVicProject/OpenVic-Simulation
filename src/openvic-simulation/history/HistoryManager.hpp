#pragma once

#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/history/DiplomaticHistory.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct HistoryManager {
	private:
		BookmarkManager bookmark_manager;
		CountryHistoryManager country_manager;
		ProvinceHistoryManager province_manager;
		DiplomaticHistoryManager diplomacy_manager;

	public:
		REF_GETTERS(bookmark_manager)
		REF_GETTERS(country_manager)
		REF_GETTERS(province_manager)
		REF_GETTERS(diplomacy_manager)
	};
}
