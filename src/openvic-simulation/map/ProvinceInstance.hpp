#pragma once

#include <plf_colony.h>

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/map/LifeRating.hpp"
#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopIdInProvince.hpp"
#include "openvic-simulation/population/PopsAggregate.hpp"
#include "openvic-simulation/types/ColonyStatus.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/FixedVector.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/TypedSpan.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct BuildingType;
	struct CountryInstance;
	struct CountryInstanceManager;
	struct CountryParty;
	struct Crime;
	struct Culture;
	struct GameRulesManager;
	struct GoodDefinition;
	struct Ideology;
	struct InstanceManager;
	struct IssueManager;
	struct MapInstance;
	struct MilitaryDefines;
	struct PopDeps;
	struct ProvinceDefinition;
	struct ProvinceHistoryEntry;
	struct ProvinceInstanceDeps;
	struct Religion;
	struct ResourceGatheringOperationDeps;
	struct State;
	struct StaticModifierCache;
	struct Strata;
	struct TerrainType;
	struct UnitInstanceGroup;

	//HasIndex index_t must match ProvinceDefinition's index_t
	struct ProvinceInstance
		: HasIdentifierAndColour,
		HasIndex<ProvinceInstance, province_index_t>,
		FlagStrings,
		PopsAggregate {
		friend struct MapInstance;

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
		GameRulesManager const& game_rules_manager;

		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating, life_rating_t { 0 });
		colony_status_t PROPERTY(colony_status, colony_status_t::STATE);
		State* PROPERTY_PTR(state, nullptr);

		CountryInstance* PROPERTY_PTR(owner, nullptr);
		ModifierSum const& get_owner_modifier_sum() const;
		CountryInstance* PROPERTY_PTR(controller, nullptr);
		CountryInstance* PROPERTY_PTR(country_to_report_economy, nullptr);
		ordered_set<CountryInstance*> PROPERTY(cores);

		// The total/resultant modifier of local effects on this province (global effects come from the province's owner)
		ModifierSum PROPERTY(modifier_sum);
		memory::vector<ModifierInstance> SPAN_PROPERTY(event_modifiers);

		bool PROPERTY(slave, false);
		// Used for "minorities = yes/no" condition
		bool PROPERTY(has_unaccepted_pops, false);
		bool PROPERTY_RW(connected_to_capital, false);
		bool PROPERTY_RW(is_overseas, false);
		bool PROPERTY(has_empty_adjacent_province, false);
		memory::vector<ProvinceInstance const*> SPAN_PROPERTY(adjacent_nonempty_land_provinces);
		Crime const* PROPERTY_RW(crime, nullptr);
		ResourceGatheringOperation PROPERTY(rgo);
		memory::FixedVector<BuildingInstance> _buildings;
		TypedSpan<province_building_index_t, BuildingInstance> buildings;
	public:
		constexpr TypedSpan<province_building_index_t, const BuildingInstance> get_buildings() const {
			return buildings;
		}
	private:
		memory::vector<ArmyInstance*> SPAN_PROPERTY(armies);
		memory::vector<NavyInstance*> SPAN_PROPERTY(navies);
		// The number of land regiments currently in the province, including those being transported by navies
		size_t PROPERTY(land_regiment_count, 0);
		Timespan PROPERTY(occupation_duration);

	public:
		OV_UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		OV_UNIT_BRANCHED_GETTER_CONST(get_unit_instance_groups, armies, navies);

	private:
		pop_id_in_province_t last_pop_id{0};
		memory::colony<Pop> PROPERTY(pops); // TODO - replace with a more easily vectorisable container?		
		void _add_pop(Pop&& pop);
		void _update_pops(MilitaryDefines const& military_defines);
		bool convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type);
		void initialise_rgo();

		OV_IFLATMAP_PROPERTY(PopType, memory::vector<Pop*>, pops_cache_by_type);
	public:
		ProvinceDefinition const& province_definition;

		//pointers instead of references to allow construction via std::tuple
		ProvinceInstance(
			ProvinceDefinition const* new_province_definition,
			ProvinceInstanceDeps const* province_instance_deps
		);
		ProvinceInstance(ProvinceInstance const&) = delete;
		ProvinceInstance& operator=(ProvinceInstance const&) = delete;
		ProvinceInstance(ProvinceInstance&&) = delete;
		ProvinceInstance& operator=(ProvinceInstance&&) = delete;

		inline explicit constexpr operator ProvinceDefinition const&() const {
			return province_definition;
		}

		void set_state(State* new_state);

		GoodDefinition const* get_rgo_good() const;
		bool set_rgo_production_type_nullable(ProductionType const* rgo_production_type_nullable);

		bool set_owner(CountryInstance* new_owner);
		bool set_controller(CountryInstance* new_controller);

		// The warn argument controls whether a log message is emitted when a core already does/doesn't exist, e.g. we may
		// want to know if there are redundant province history instructions setting a core multiple times, but we may not
		// want to be swamped with log messages by effect scripts which use add_core/remove_core very liberally. Regardless
		// of warn's value, both functions will still return true if the core in question already does/doesn't exist.
		bool add_core(CountryInstance& new_core, bool warn);
		bool remove_core(CountryInstance& core_to_remove, bool warn);
		constexpr bool is_owner_core() const {
			return owner != nullptr && cores.contains(owner);
		}
		constexpr bool is_colonial_province() const {
			return is_colonial(colony_status);
		}
		constexpr bool is_occupied() const {
			return owner != controller;
		}
		constexpr bool is_empty() const {
			return owner == nullptr;
		}

		constexpr bool may_build_here() const {
			return !is_occupied();
		}
		bool expand_building(
			ModifierEffectCache const& modifier_effect_cache,
			const province_building_index_t index,
			CountryInstance& actor
		);

		bool add_pop_vec(
			std::span<const PopBase> pop_vec,
			PopDeps const& pop_deps
		);
		size_t get_pop_count() const;

		void update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache);
		fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const;

		void for_each_contributing_modifier(ModifierEffect const& effect, ContributingModifierCallback auto callback) const {
			if (effect.is_local()) {
				// Province-targeted/local effects come from the province itself, only modifiers applied directly to the
				// province contribute to these effects.
				modifier_sum.for_each_contributing_modifier(effect, std::move(callback));
			} else if (owner != nullptr) {
				// Non-province targeted/global effects come from the province's owner, even those applied locally
				// (e.g. via a province event modifier) are passed up to the province's controller and only affect the
				// province if the controller is also the owner.
				get_owner_modifier_sum().for_each_contributing_modifier(effect, std::move(callback));
			}
		}
		void update_gamestate(InstanceManager const& instance_manager);
		static constexpr size_t VECTORS_FOR_PROVINCE_TICK = std::max(
			ResourceGatheringOperation::VECTORS_FOR_RGO_TICK,
			Pop::VECTORS_FOR_POP_TICK
		);
		void province_tick(
			const Date today,
			PopValuesFromProvince& reusable_pop_values,
			RandomU32& random_number_generator,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_PROVINCE_TICK
			> reusable_vectors
		);
		void initialise_for_new_game(
			const Date today,
			PopValuesFromProvince& reusable_pop_values,
			RandomU32& random_number_generator,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_PROVINCE_TICK
			> reusable_vectors
		);

		bool evaluate_leaf(ConditionNode const& node) const;

		bool add_unit_instance_group(UnitInstanceGroup& group);
		bool remove_unit_instance_group(UnitInstanceGroup const& group);

		bool apply_history_to_province(ProvinceHistoryEntry const& entry, CountryInstanceManager& country_manager);

		void setup_pop_test_values(IssueManager const& issue_manager);
		memory::colony<Pop>& get_mutable_pops();
	private:
		template<typename T>
		static std::conditional_t<std::is_const_v<T>, Pop const*, Pop*> _find_pop_by_id(T& self, const pop_id_in_province_t pop_id);
	public:
		Pop* find_pop_by_id(const pop_id_in_province_t pop_id);
		Pop const* find_pop_by_id(const pop_id_in_province_t pop_id) const;
	};
}