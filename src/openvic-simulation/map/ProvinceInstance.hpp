#pragma once

#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct MapInstance;
	struct ProvinceDefinition;
	struct TerrainType;
	struct State;
	struct Country;
	struct Crime;
	struct Good;
	struct ArmyInstance;
	struct NavyInstance;
	struct Ideology;
	struct Culture;
	struct Religion;
	struct BuildingTypeManager;
	struct ProvinceHistoryEntry;
	struct IdeologyManager;
	struct IssueManager;

	struct ProvinceInstance : HasIdentifier {
		friend struct MapInstance;

		using life_rating_t = int8_t;

		enum struct colony_status_t : uint8_t { STATE, PROTECTORATE, COLONY };

		ProvinceDefinition const& PROPERTY(province_definition);

		/* Mutable attributes (reset before loading history) */
		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating);
		colony_status_t PROPERTY(colony_status);
		State const* PROPERTY_RW(state);
		Country const* PROPERTY(owner);
		Country const* PROPERTY(controller);
		std::vector<Country const*> PROPERTY(cores);
		bool PROPERTY(slave);
		Crime const* PROPERTY_RW(crime);
		// TODO - change this into a factory-like structure
		Good const* PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> IDENTIFIER_REGISTRY(building);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);

		std::vector<Pop> PROPERTY(pops);
		Pop::pop_size_t PROPERTY(total_population);
		fixed_point_map_t<PopType const*> PROPERTY(pop_type_distribution);
		fixed_point_map_t<Ideology const*> PROPERTY(ideology_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);

		ProvinceInstance(ProvinceDefinition const& new_province_definition);

		void _add_pop(Pop pop);
		void _update_pops();

	public:
		ProvinceInstance(ProvinceInstance&&) = default;

		inline explicit constexpr operator ProvinceDefinition const&() const {
			return province_definition;
		}

		bool expand_building(size_t building_index);

		bool add_pop(Pop&& pop);
		bool add_pop_vec(std::vector<Pop> const& pop_vec);
		size_t get_pop_count() const;

		void update_gamestate(Date today);
		void tick(Date today);

		bool add_army(ArmyInstance& army);
		bool remove_army(ArmyInstance& army);
		bool add_navy(NavyInstance& navy);
		bool remove_navy(NavyInstance& navy);

		bool reset(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const* entry);

		void setup_pop_test_values(
			IdeologyManager const& ideology_manager, IssueManager const& issue_manager, Country const& country
		);
	};
}
