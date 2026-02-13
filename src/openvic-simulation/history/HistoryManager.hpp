#pragma once

#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/history/DiplomaticHistory.hpp"
#include "openvic-simulation/history/ProvinceHistory.hpp"

namespace OpenVic {
	struct DefinitionManager;

	struct HistoryManager {
	private:
		BookmarkManager PROPERTY_REF(bookmark_manager);
		CountryHistoryManager PROPERTY_REF(country_manager);
		ProvinceHistoryManager PROPERTY_REF(province_manager);
		DiplomaticHistoryManager PROPERTY_REF(diplomacy_manager);
	public:
		HistoryManager(DefinitionManager const& definition_manager) : province_manager { definition_manager } {}
	};
}
