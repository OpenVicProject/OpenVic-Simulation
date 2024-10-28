#pragma once

#include <plf_colony.h>

#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct MapInstance;
	struct ProvinceDefinition;
	struct TerrainType;
	struct State;
	struct CountryInstance;
	struct Crime;
	struct GoodDefinition;
	struct Ideology;
	struct Culture;
	struct Religion;
	struct StaticModifierCache;
	struct BuildingTypeManager;
	struct ProvinceHistoryEntry;
	struct IssueManager;
	struct CountryInstanceManager;

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

		static constexpr std::string_view get_colony_status_string(colony_status_t colony_status) {
			using enum colony_status_t;
			switch (colony_status) {
			case STATE:
				return "state";
			case PROTECTORATE:
				return "protectorate";
			case COLONY:
				return "colony";
			default:
				return "unknown colony status";
			}
		}

	private:
		ProvinceDefinition const& PROPERTY(province_definition);

		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating);
		colony_status_t PROPERTY(colony_status);
		State* PROPERTY_RW(state);

		CountryInstance* PROPERTY(owner);
		CountryInstance* PROPERTY(controller);
		ordered_set<CountryInstance*> PROPERTY(cores);

	public:
		static constexpr bool ADD_OWNER_CONTRIBUTION = true;

	private:
		// The total/resultant modifier affecting this province, including owner country contributions if
		// ADD_OWNER_CONTRIBUTION is true.
		ModifierSum PROPERTY(modifier_sum);
		std::vector<ModifierInstance> PROPERTY(event_modifiers);

		bool PROPERTY(slave);
		Crime const* PROPERTY_RW(crime);
		ResourceGatheringOperation PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> IDENTIFIER_REGISTRY(building);
		ordered_set<ArmyInstance*> PROPERTY(armies);
		ordered_set<NavyInstance*> PROPERTY(navies);

		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);

		plf::colony<Pop> PROPERTY(pops); // TODO - replace with a more easily vectorisable container?
		Pop::pop_size_t PROPERTY(total_population);
		// TODO - population change (growth + migration), monthly totals + breakdown by source/destination
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);
		IndexedMap<PopType, Pop::pop_size_t> PROPERTY(pop_type_distribution);
		IndexedMap<PopType, std::vector<Pop*>> PROPERTY(pops_cache_by_type);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);
		size_t PROPERTY(max_supported_regiments);

		ProvinceInstance(
			MarketInstance& new_market_instance,
			ModifierEffectCache const& new_modifier_effect_cache,
			ProvinceDefinition const& new_province_definition,
			decltype(pop_type_distribution)::keys_t const& pop_type_keys,
			decltype(ideology_distribution)::keys_t const& ideology_keys
		);

		void _add_pop(Pop&& pop);
		void _update_pops(DefineManager const& define_manager);
		bool convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type);

	public:
		ProvinceInstance(ProvinceInstance&&) = default;

		inline explicit constexpr operator ProvinceDefinition const&() const {
			return province_definition;
		}

		constexpr CountryInstance* get_owner() {
			return owner;
		}
		constexpr CountryInstance* get_controller() {
			return controller;
		}

		GoodDefinition const* get_rgo_good() const;
		bool set_rgo_production_type_nullable(ProductionType const* rgo_production_type_nullable);

		bool set_owner(CountryInstance* new_owner);
		bool set_controller(CountryInstance* new_controller);
		bool add_core(CountryInstance& new_core);
		bool remove_core(CountryInstance& core_to_remove);
		bool is_owner_core() const;

		bool expand_building(size_t building_index);

		bool add_pop(Pop&& pop);
		bool add_pop_vec(std::vector<PopBase> const& pop_vec);
		size_t get_pop_count() const;

		void update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache);
		void contribute_country_modifier_sum(ModifierSum const& owner_modifier_sum);
		fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const;
		fixed_point_t get_modifier_effect_value_nullcheck(ModifierEffect const* effect) const;
		void push_contributing_modifiers(
			ModifierEffect const& effect, std::vector<ModifierSum::modifier_entry_t>& contributions
		) const;
		std::vector<ModifierSum::modifier_entry_t> get_contributing_modifiers(ModifierEffect const& effect) const;

		void update_gamestate(const Date today, DefineManager const& define_manager);
		void province_tick(const Date today);

		template<UnitType::branch_t Branch>
		bool add_unit_instance_group(UnitInstanceGroup<Branch>& group);
		template<UnitType::branch_t Branch>
		bool remove_unit_instance_group(UnitInstanceGroup<Branch>& group);

		bool setup(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const& entry, CountryInstanceManager& country_manager);

		void initialise_rgo();

		void setup_pop_test_values(IssueManager const& issue_manager);
		State* get_mutable_state();
		plf::colony<Pop>& get_mutable_pops();
		IndexedMap<PopType, std::vector<Pop*>>& get_mutable_pops_cache_by_type();
	};
}
