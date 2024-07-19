#pragma once

#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct MapInstance;
	struct ProvinceDefinition;
	struct TerrainType;
	struct State;
	struct CountryDefinition;
	struct Crime;
	struct GoodDefinition;
	struct Ideology;
	struct Culture;
	struct Religion;
	struct BuildingTypeManager;
	struct ProvinceHistoryEntry;
	struct IssueManager;

	template<UnitType::branch_t>
	struct UnitInstanceGroup;

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	using ArmyInstance = UnitInstanceGroupBranched<UnitType::branch_t::LAND>;
	using NavyInstance = UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>;

	struct ProvinceInstance : HasIdentifierAndColour {
		friend struct MapInstance;

		using life_rating_t = int8_t;

		enum struct colony_status_t : uint8_t { STATE, PROTECTORATE, COLONY };

	private:
		ProvinceDefinition const& PROPERTY(province_definition);

		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating);
		colony_status_t PROPERTY(colony_status);
		State const* PROPERTY_RW(state);
		CountryDefinition const* PROPERTY(owner);
		CountryDefinition const* PROPERTY(controller);
		// Cores being CountryDefinitions means then they won't be affected by tag switched (as desired)
		std::vector<CountryDefinition const*> PROPERTY(cores);
		bool PROPERTY(slave);
		Crime const* PROPERTY_RW(crime);
		// TODO - change this into a factory-like structure
		GoodDefinition const* PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> IDENTIFIER_REGISTRY(building);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);

		std::vector<Pop> PROPERTY(pops);
		Pop::pop_size_t PROPERTY(total_population);
		IndexedMap<PopType, fixed_point_t> PROPERTY(pop_type_distribution);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);

		ProvinceInstance(
			ProvinceDefinition const& new_province_definition, decltype(pop_type_distribution)::keys_t const& pop_type_keys,
			decltype(ideology_distribution)::keys_t const& ideology_keys
		);

		void _add_pop(Pop&& pop);
		void _update_pops();

	public:
		ProvinceInstance(ProvinceInstance&&) = default;

		inline explicit constexpr operator ProvinceDefinition const&() const {
			return province_definition;
		}

		bool expand_building(size_t building_index);

		bool add_pop(Pop&& pop);
		bool add_pop_vec(std::vector<PopBase> const& pop_vec);
		size_t get_pop_count() const;

		void update_gamestate(Date today);
		void tick(Date today);

		template<UnitType::branch_t Branch>
		bool add_unit_instance_group(UnitInstanceGroup<Branch>& group);
		template<UnitType::branch_t Branch>
		bool remove_unit_instance_group(UnitInstanceGroup<Branch>& group);

		bool setup(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const* entry);

		void setup_pop_test_values(IssueManager const& issue_manager);
	};
}
