#pragma once

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct MapDefinition;
	struct BuildingTypeManager;
	struct ProvinceHistoryManager;
	struct IdeologyManager;
	struct IssueManager;
	struct Country;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct MapInstance {
		MapDefinition const& PROPERTY(map_definition);

		IdentifierRegistry<ProvinceInstance> IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(province_instance, 1);

		ProvinceInstance* PROPERTY(selected_province); // is it right for this to be mutable? how about using an index instead?
		Pop::pop_size_t PROPERTY(highest_province_population);
		Pop::pop_size_t PROPERTY(total_map_population);

		StateManager PROPERTY_REF(state_manager);

	public:
		MapInstance(MapDefinition const& new_map_definition);

		inline explicit constexpr operator MapDefinition const&() const {
			return map_definition;
		}

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(province_instance, 1);

		ProvinceInstance* get_province_instance_from_const(ProvinceDefinition const* province);
		ProvinceInstance const* get_province_instance_from_const(ProvinceDefinition const* province) const;

		void set_selected_province(ProvinceDefinition::index_t index);
		ProvinceInstance* get_selected_province();
		ProvinceDefinition::index_t get_selected_province_index() const;

		bool setup(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_provinces(
			ProvinceHistoryManager const& history_manager, Date date, IdeologyManager const& ideology_manager,
			IssueManager const& issue_manager, Country const& country
		);

		void update_gamestate(Date today);
		void tick(Date today);
	};
}
