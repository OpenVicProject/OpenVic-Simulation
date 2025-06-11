#pragma once

#include <span>

#include <plf_colony.h>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperation.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/modifier/ModifierSum.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/FlagStrings.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct DefineManager;
	struct MapInstance;
	struct ProvinceDefinition;
	struct TerrainType;
	struct State;
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
	struct ModifierEffectCache;
	struct MarketInstance;
	struct GameRulesManager;

	struct UnitInstanceGroup;

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	using ArmyInstance = UnitInstanceGroupBranched<UnitType::branch_t::LAND>;
	using NavyInstance = UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>;

	struct ProvinceInstance : HasIdentifierAndColour, HasIndex<>, FlagStrings {
		friend struct MapInstance;

		using life_rating_t = int8_t;

		enum struct colony_status_t : uint8_t { STATE, PROTECTORATE, COLONY };

		// This combines COLONY and PROTECTORATE statuses, as opposed to non-colonial STATE provinces
		static constexpr bool is_colonial(colony_status_t colony_status) {
			return colony_status != colony_status_t::STATE;
		}

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
		GameRulesManager const& PROPERTY(game_rules_manager);
		ModifierEffectCache const& PROPERTY(modifier_effect_cache);

		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating, 0);
		colony_status_t PROPERTY(colony_status, colony_status_t::STATE);
		State* PROPERTY_PTR(state, nullptr);

		CountryInstance* PROPERTY_PTR(owner, nullptr);
		CountryInstance* PROPERTY_PTR(controller, nullptr);
		CountryInstance* PROPERTY_PTR(country_to_report_economy, nullptr);
		ordered_set<CountryInstance*> PROPERTY(cores);

		// The total/resultant modifier of local effects on this province (global effects come from the province's owner)
		ModifierSum PROPERTY(modifier_sum);
		std::vector<ModifierInstance> PROPERTY(event_modifiers);

		bool PROPERTY(slave, false);
		// Used for "minorities = yes/no" condition
		bool PROPERTY(has_unaccepted_pops, false);
		bool PROPERTY_RW(connected_to_capital, false);
		bool PROPERTY_RW(is_overseas, false);
		bool PROPERTY(has_empty_adjacent_province, false);
		std::vector<ProvinceInstance const*> PROPERTY(adjacent_nonempty_land_provinces);
		Crime const* PROPERTY_RW(crime, nullptr);
		ResourceGatheringOperation PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> IDENTIFIER_REGISTRY(building);
		std::vector<ArmyInstance*> PROPERTY(armies);
		std::vector<NavyInstance*> PROPERTY(navies);
		// The number of land regiments currently in the province, including those being transported by navies
		size_t PROPERTY(land_regiment_count, 0);
		Timespan PROPERTY(occupation_duration);

	public:
		UNIT_BRANCHED_GETTER(get_unit_instance_groups, armies, navies);
		UNIT_BRANCHED_GETTER_CONST(get_unit_instance_groups, armies, navies);

	private:
		plf::colony<Pop> PROPERTY(pops); // TODO - replace with a more easily vectorisable container?
		pop_size_t PROPERTY(total_population, 0);
		fixed_point_t PROPERTY(yesterdays_import_value);
		// TODO - population change (growth + migration), monthly totals + breakdown by source/destination
		fixed_point_t PROPERTY(average_literacy);
		fixed_point_t PROPERTY(average_consciousness);
		fixed_point_t PROPERTY(average_militancy);

		IndexedMap<Strata, pop_size_t> PROPERTY(population_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(militancy_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(life_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(everyday_needs_fulfilled_by_strata);
		IndexedMap<Strata, fixed_point_t> PROPERTY(luxury_needs_fulfilled_by_strata);

		IndexedMap<PopType, pop_size_t> PROPERTY(pop_type_distribution);
		IndexedMap<PopType, pop_size_t> PROPERTY(pop_type_unemployed_count);
		IndexedMap<PopType, std::vector<Pop*>> PROPERTY(pops_cache_by_type);
		IndexedMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Issue const*> PROPERTY(issue_distribution);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(vote_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);
		size_t PROPERTY(max_supported_regiments, 0);

		ProvinceInstance(
			MarketInstance& new_market_instance,
			GameRulesManager const& new_game_rules_manager,
			ModifierEffectCache const& new_modifier_effect_cache,
			ProvinceDefinition const& new_province_definition,
			decltype(population_by_strata)::keys_type const& strata_keys,
			decltype(pop_type_distribution)::keys_type const& pop_type_keys,
			decltype(ideology_distribution)::keys_type const& ideology_keys
		);

		void _add_pop(Pop&& pop);
		void _update_pops(DefineManager const& define_manager);
		bool convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type);
		void initialise_rgo();

	public:
		ProvinceInstance(ProvinceInstance&&) = default;

		inline explicit constexpr operator ProvinceDefinition const&() const {
			return province_definition;
		}

		void set_state(State* new_state);

		constexpr GoodDefinition const* get_rgo_good() const {
			if (!rgo.is_valid()) {
				return nullptr;
			}
			return &(rgo.get_production_type_nullable()->get_output_good());
		}

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

		// The values returned by these functions are scaled by population size, so they must be divided by population size
		// to get the support as a proportion of 1.0

		constexpr pop_size_t get_pop_type_proportion(PopType const& pop_type) const {
			return pop_type_distribution[pop_type];
		}
		constexpr pop_size_t get_pop_type_unemployed(PopType const& pop_type) const {
			return pop_type_unemployed_count[pop_type];
		}
		constexpr fixed_point_t get_ideology_support(Ideology const& ideology) const {
			return ideology_distribution[ideology];
		}
		fixed_point_t get_issue_support(Issue const& issue) const;
		fixed_point_t get_party_support(CountryParty const& party) const;
		fixed_point_t get_culture_proportion(Culture const& culture) const;
		fixed_point_t get_religion_proportion(Religion const& religion) const;
		constexpr pop_size_t get_strata_population(Strata const& strata) const {
			return population_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_militancy(Strata const& strata) const {
			return militancy_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_life_needs_fulfilled(Strata const& strata) const {
			return life_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_everyday_needs_fulfilled(Strata const& strata) const {
			return everyday_needs_fulfilled_by_strata[strata];
		}
		constexpr fixed_point_t get_strata_luxury_needs_fulfilled(Strata const& strata) const {
			return luxury_needs_fulfilled_by_strata[strata];
		}

		bool expand_building(size_t building_index);

		bool add_pop(Pop&& pop);
		bool add_pop_vec(
			std::span<const PopBase> pop_vec,
			MarketInstance& market_instance,
			ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
		);
		size_t get_pop_count() const;

		void update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache);
		fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const;
		constexpr void for_each_contributing_modifier(
			ModifierEffect const& effect, ContributingModifierCallback auto callback
		) const {
			using enum ModifierEffect::target_t;

			if (effect.is_local()) {
				// Province-targeted/local effects come from the province itself, only modifiers applied directly to the
				// province contribute to these effects.
				modifier_sum.for_each_contributing_modifier(effect, std::move(callback));
			} else if (owner != nullptr) {
				// Non-province targeted/global effects come from the province's owner, even those applied locally
				// (e.g. via a province event modifier) are passed up to the province's controller and only affect the
				// province if the controller is also the owner.
				owner->for_each_contributing_modifier(effect, std::move(callback));
			}
		}

		void update_gamestate(InstanceManager const& instance_manager);
		void province_tick(
			const Date today,
			PopValuesFromProvince& reusable_pop_values,
			std::vector<fixed_point_t>& reusable_vector
		);
		void initialise_for_new_game(
			const Date today,
			PopValuesFromProvince& reusable_pop_values,
			std::vector<fixed_point_t>& reusable_vector
		);

		bool add_unit_instance_group(UnitInstanceGroup& group);
		bool remove_unit_instance_group(UnitInstanceGroup const& group);

		bool setup(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const& entry, CountryInstanceManager& country_manager);

		void setup_pop_test_values(IssueManager const& issue_manager);
		plf::colony<Pop>& get_mutable_pops();
	};
}
