#pragma once

#include <optional>
#include <vector>

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ProvinceHistoryMap;
	struct ProvinceDefinition;
	struct Country;
	struct GoodDefinition;
	struct TerrainType;
	struct Ideology;
	struct GameManager;

	struct ProvinceHistoryEntry : HistoryEntry {
		friend struct ProvinceHistoryMap;

	private:
		ProvinceDefinition const& PROPERTY(province);

		std::optional<Country const*> PROPERTY(owner);
		std::optional<Country const*> PROPERTY(controller);
		std::optional<ProvinceInstance::colony_status_t> PROPERTY(colonial);
		std::optional<bool> PROPERTY(slave);
		std::vector<Country const*> PROPERTY(add_cores);
		std::vector<Country const*> PROPERTY(remove_cores);
		std::optional<GoodDefinition const*> PROPERTY(rgo);
		std::optional<ProvinceInstance::life_rating_t> PROPERTY(life_rating);
		std::optional<TerrainType const*> PROPERTY(terrain_type);
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(province_buildings);
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(state_buildings);
		fixed_point_map_t<Ideology const*> PROPERTY(party_loyalties);

		// TODO - use minimal pop representation (size, type, culture, religion, consciousness, militancy, rebel type)
		std::vector<Pop> PROPERTY(pops);

		ProvinceHistoryEntry(ProvinceDefinition const& new_province, Date new_date);

		bool _load_province_pop_history(GameManager const& game_manager, ast::NodeCPtr root, bool *non_integer_size);
	};

	struct ProvinceHistoryManager;

	struct ProvinceHistoryMap : HistoryMap<ProvinceHistoryEntry> {
		friend struct ProvinceHistoryManager;

	private:
		ProvinceDefinition const& PROPERTY(province);

	protected:
		ProvinceHistoryMap(ProvinceDefinition const& new_province);

		std::unique_ptr<ProvinceHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(GameManager const& game_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root) override;

	private:
		bool _load_province_pop_history(
			GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
		);
	};

	struct MapDefinition;

	struct ProvinceHistoryManager {
	private:
		ordered_map<ProvinceDefinition const*, ProvinceHistoryMap> province_histories;
		bool locked = false;

		ProvinceHistoryMap* _get_or_make_province_history(ProvinceDefinition const& province);

	public:
		ProvinceHistoryManager() = default;

		void reserve_more_province_histories(size_t size);
		void lock_province_histories(MapDefinition const& map_definition, bool detailed_errors);
		bool is_locked() const;

		ProvinceHistoryMap const* get_province_history(ProvinceDefinition const* province) const;

		bool load_province_history_file(
			GameManager const& game_manager, ProvinceDefinition const& province, ast::NodeCPtr root
		);
		bool load_pop_history_file(GameManager const& game_manager, Date date, ast::NodeCPtr root, bool *non_integer_size);
	};
}
