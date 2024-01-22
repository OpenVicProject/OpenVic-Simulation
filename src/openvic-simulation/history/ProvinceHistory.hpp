#pragma once

#include <vector>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
#include "openvic-simulation/map/Province.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct ProvinceHistoryMap;

	struct ProvinceHistoryEntry : HistoryEntry {
		friend struct ProvinceHistoryMap;

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
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(province_buildings);
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(state_buildings);
		fixed_point_map_t<Ideology const*> PROPERTY(party_loyalties);
		std::vector<Pop> PROPERTY(pops);

		ProvinceHistoryEntry(Province const& new_province, Date new_date);

		bool _load_province_pop_history(GameManager const& game_manager, ast::NodeCPtr root, bool *non_integer_size);
	};

	struct ProvinceHistoryManager;

	struct ProvinceHistoryMap : HistoryMap<ProvinceHistoryEntry> {
		friend struct ProvinceHistoryManager;

	private:
		Province const& PROPERTY(province);

	protected:
		ProvinceHistoryMap(Province const& new_province);

		std::unique_ptr<ProvinceHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(GameManager const& game_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root) override;

	private:
		bool _load_province_pop_history(
			GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
		);
	};

	struct ProvinceHistoryManager {
	private:
		ordered_map<Province const*, ProvinceHistoryMap> province_histories;
		bool locked = false;

		ProvinceHistoryMap* _get_or_make_province_history(Province const& province);

	public:
		ProvinceHistoryManager() = default;

		void reserve_more_province_histories(size_t size);
		void lock_province_histories(Map const& map, bool detailed_errors);
		bool is_locked() const;

		ProvinceHistoryMap const* get_province_history(Province const* province) const;

		bool load_province_history_file(GameManager const& game_manager, Province const& province, ast::NodeCPtr root);
		bool load_pop_history_file(GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size);
	};
} // namespace OpenVic
