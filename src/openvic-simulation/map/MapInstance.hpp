#pragma once

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/pathfinding/AStarPathing.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct IssueManager;
	struct MapDefinition;
	struct MarketInstance;
	struct PopDeps;
	struct ProvinceHistoryManager;
	struct ProvinceInstanceDeps;
	struct ThreadPool;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct MapInstance {
		using canal_index_t = ProvinceDefinition::adjacency_t::data_t;

	private:
		MapDefinition const& map_definition;
		ThreadPool& thread_pool;

		OV_IFLATMAP_PROPERTY(ProvinceDefinition, ProvinceInstance, province_instance_by_definition);

		pop_size_t PROPERTY(highest_province_population, 0);
		pop_size_t PROPERTY(total_map_population, 0);

		StateManager PROPERTY_REF(state_manager);
		// TODO - should this be a vector of bools which we resize to the largest enabled canal index?
		ordered_set<canal_index_t> PROPERTY(enabled_canals);

		ArmyAStarPathing PROPERTY_REF(land_pathing);
		NavyAStarPathing PROPERTY_REF(sea_pathing);

		OV_SPEED_INLINE bool apply_history_to_province(
			ProvinceHistoryManager const& history_manager,
			const Date date,
			CountryInstanceManager& country_manager,
			IssueManager const& issue_manager,
			PopDeps const& pop_deps,
			ProvinceInstance& province
		);

	public:
		MapInstance(
			MapDefinition const& new_map_definition,
			ProvinceInstanceDeps const& province_instance_deps,
			ThreadPool& new_thread_pool
		);

		inline explicit constexpr operator MapDefinition const&() const {
			return map_definition;
		}

		constexpr forwardable_span<ProvinceInstance> get_province_instances() {
			return province_instance_by_definition.get_values();
		}

		constexpr forwardable_span<const ProvinceInstance> get_province_instances() const {
			return province_instance_by_definition.get_values();
		}
		ProvinceInstance* get_province_instance_by_identifier(std::string_view identifier);
		ProvinceInstance const* get_province_instance_by_identifier(std::string_view identifier) const;
		ProvinceInstance* get_province_instance_by_index(typename ProvinceInstance::index_t index);
		ProvinceInstance const* get_province_instance_by_index(typename ProvinceInstance::index_t index) const;
		ProvinceInstance& get_province_instance_by_definition(ProvinceDefinition const& province_definition);
		ProvinceInstance* get_province_instance_from_number(
			decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
		);
		ProvinceInstance const* get_province_instance_from_number(
			decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
		) const;

		void enable_canal(canal_index_t canal_index);
		bool is_canal_enabled(canal_index_t canal_index) const;

		bool apply_history_to_provinces(
			ProvinceHistoryManager const& history_manager,
			const Date date,
			CountryInstanceManager& country_manager,
			IssueManager const& issue_manager,
			PopDeps const& pop_deps
		);

		void update_modifier_sums(const Date today, StaticModifierCache const& static_modifier_cache);
		void update_gamestate(InstanceManager const& instance_manager);
		void map_tick();
		void initialise_for_new_game(InstanceManager const& instance_manager);
	};
}
