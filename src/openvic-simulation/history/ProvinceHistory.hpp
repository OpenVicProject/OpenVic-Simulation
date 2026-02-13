#pragma once

#include <optional>

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/history/HistoryMap.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/economy/BuildingLevel.hpp"
#include "openvic-simulation/types/ColonyStatus.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/FixedVector.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedSpan.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/map/LifeRating.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct CountryDefinition;
	struct DefinitionManager;
	struct GoodDefinition;
	struct Ideology;
	struct ProvinceDefinition;
	struct ProvinceHistoryMap;
	struct TerrainType;

	struct ProvinceHistoryEntry : HistoryEntry {
		friend struct ProvinceHistoryMap;

		ProvinceHistoryEntry(
			BuildingTypeManager const& building_type_manager,
			ProvinceDefinition const& new_province,
			Date new_date
		);
	private:
		std::optional<CountryDefinition const*> PROPERTY(owner);
		std::optional<CountryDefinition const*> PROPERTY(controller);
		std::optional<colony_status_t> PROPERTY(colonial);
		std::optional<bool> PROPERTY(slave);
		ordered_map<CountryDefinition const*, bool> PROPERTY(cores);
		std::optional<ProductionType const*> PROPERTY(rgo_production_type_nullable);
		std::optional<life_rating_t> PROPERTY(life_rating);
		std::optional<TerrainType const*> PROPERTY(terrain_type);
		memory::FixedVector<building_level_t> _province_building_levels;
		TypedSpan<province_building_index_t, building_level_t> province_building_levels;
	public:
		constexpr TypedSpan<province_building_index_t, const building_level_t> get_province_building_levels() const {
			return province_building_levels;
		}
	private:
		ordered_map<building_type_index_t, building_level_t> PROPERTY(state_buildings);
		fixed_point_map_t<Ideology const*> PROPERTY(party_loyalties);
		memory::vector<PopBase> SPAN_PROPERTY(pops);

		bool _load_province_pop_history(
			DefinitionManager const& definition_manager, ast::NodeCPtr root, bool *non_integer_size
		);
	public:
		ProvinceDefinition const& province;
	};

	struct ProvinceHistoryManager;

	struct ProvinceHistoryMap : HistoryMap<ProvinceHistoryEntry> {
		friend struct ProvinceHistoryManager;

	private:
		ProvinceDefinition const& province;
		BuildingTypeManager const& building_type_manager;

	protected:
		ProvinceHistoryMap(ProvinceDefinition const& new_province, BuildingTypeManager const& new_building_type_manager);

		memory::unique_ptr<ProvinceHistoryEntry> _make_entry(Date date) const override;
		bool _load_history_entry(DefinitionManager const& definition_manager, ProvinceHistoryEntry& entry, ast::NodeCPtr root)
			override;

	private:
		bool _load_province_pop_history(
			DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool* non_integer_size
		);
	};

	struct MapDefinition;

	struct ProvinceHistoryManager {
	private:
		DefinitionManager const& definition_manager;
		ordered_map<ProvinceDefinition const*, ProvinceHistoryMap> province_histories;
		bool locked = false;

		ProvinceHistoryMap* _get_or_make_province_history(ProvinceDefinition const& province);

	public:
		ProvinceHistoryManager(DefinitionManager const& new_definition_manager) : definition_manager { new_definition_manager } {};

		void reserve_more_province_histories(size_t size);
		void lock_province_histories(MapDefinition const& map_definition, bool detailed_errors);
		bool is_locked() const;

		ProvinceHistoryMap const* get_province_history(ProvinceDefinition const* province) const;

		bool load_province_history_file(
			DefinitionManager const& definition_manager, ProvinceDefinition const& province, ast::NodeCPtr root
		);
		bool load_pop_history_file(
			DefinitionManager const& definition_manager, Date date, ast::NodeCPtr root, bool* non_integer_size
		);
	};
}
