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
	struct CountryDefinition;
	struct GoodDefinition;
	struct TerrainType;
	struct Ideology;
	struct DefinitionManager;

	struct ProvinceHistoryEntry : HistoryEntry {
		friend struct ProvinceHistoryMap;

	private:
		ProvinceDefinition const& PROPERTY(province);

		std::optional<CountryDefinition const*> PROPERTY(owner);
		std::optional<CountryDefinition const*> PROPERTY(controller);
		std::optional<ProvinceInstance::colony_status_t> PROPERTY(colonial);
		std::optional<bool> PROPERTY(slave);
		ordered_map<CountryDefinition const*, bool> PROPERTY(cores);
		std::optional<ProductionType const*> PROPERTY(rgo_production_type_nullable);
		std::optional<ProvinceInstance::life_rating_t> PROPERTY(life_rating);
		std::optional<TerrainType const*> PROPERTY(terrain_type);
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(province_buildings);
		ordered_map<BuildingType const*, BuildingType::level_t> PROPERTY(state_buildings);
		fixed_point_map_t<Ideology const*> PROPERTY(party_loyalties);
		std::vector<PopBase> PROPERTY(pops);

		ProvinceHistoryEntry(ProvinceDefinition const& new_province, Date new_date);

		bool _load_province_pop_history(
			DefinitionManager const& definition_manager, ast::NodeCPtr root, bool *non_integer_size
		);
	};

	struct ProvinceHistoryManager;

	struct ProvinceHistoryMap : HistoryMap<ProvinceHistoryEntry> {
		friend struct ProvinceHistoryManager;

	private:
		ProvinceDefinition const& PROPERTY(province);

	protected:
		ProvinceHistoryMap(ProvinceDefinition const& new_province);

		std::unique_ptr<ProvinceHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(
			DefinitionManager const& definition_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root
		) override;

	private:
		bool _load_province_pop_history(
			DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
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
			DefinitionManager const& definition_manager, ProvinceDefinition const& province, ast::NodeCPtr root
		);
		bool load_pop_history_file(
			DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool *non_integer_size
		);
	};
}
