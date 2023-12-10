#pragma once

#include <bitset>
#include <map>
#include <vector>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/history/Bookmark.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/map/TerrainType.hpp"

namespace OpenVic {
	struct ProvinceHistoryMap;

	struct ProvinceHistoryEntry : HistoryEntry {
		friend struct ProvinceHistoryMap;
		friend struct ProvinceHistoryManager;

	private:
		Province const& PROPERTY(province);

		std::optional<Country const*> PROPERTY(owner);
		std::optional<Country const*> PROPERTY(controller);
		std::optional<Province::colony_status_t> PROPERTY(colonial);
		std::optional<bool> PROPERTY(slave);
		std::vector<Country const*> PROPERTY(add_cores);
		std::vector<Country const*> PROPERTY(remove_cores);
		std::optional<Good const*> PROPERTY(rgo);
		std::optional<Province::life_rating_t> PROPERTY(life_rating);
		std::optional<TerrainType const*> PROPERTY(terrain_type);
		std::map<BuildingType const*, BuildingType::level_t> PROPERTY(province_buildings);
		std::map<BuildingType const*, BuildingType::level_t> PROPERTY(state_buildings);
		fixed_point_map_t<Ideology const*> PROPERTY(party_loyalties);
		std::vector<Pop> PROPERTY(pops);

		ProvinceHistoryEntry(Province const& new_province, Date new_date);
		bool _load_province_pop_history(PopManager const& pop_manager, ast::NodeCPtr root);
	};

	struct ProvinceHistoryManager;

	struct ProvinceHistoryMap : HistoryMap<ProvinceHistoryEntry> {
		friend struct ProvinceHistoryManager;

	private:
		Province const& PROPERTY(province);

		ProvinceHistoryEntry* _get_entry(Date date);

	protected:
		ProvinceHistoryMap(Province const& new_province);

		std::unique_ptr<ProvinceHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(GameManager const& game_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root) override;
	};

	struct ProvinceHistoryManager {
	private:
		std::map<Province const*, ProvinceHistoryMap> PROPERTY(province_histories);
		bool locked = false;

	public:
		ProvinceHistoryManager() = default;

		void lock_province_histories(Map const& map, bool detailed_errors);
		bool is_locked() const;

		ProvinceHistoryMap const* get_province_history(Province const* province) const;

		bool load_province_history_file(GameManager const& game_manager, Province const& province, ast::NodeCPtr root);
		bool load_pop_history_file(GameManager const& game_manager, Date date, ast::NodeCPtr root);
	};
} // namespace OpenVic
