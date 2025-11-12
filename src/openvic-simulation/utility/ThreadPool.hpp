#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"
#include "openvic-simulation/utility/RandomGenerator.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct CountryInstance;
	struct GoodInstance;
	struct ModifierEffectCache;
	struct PopsDefines;
	struct ProductionTypeManager;
	struct Strata;
	
	//bundle work so they always have the same rng regardless of hardware concurrency
	struct WorkBundle {
	public:
		RandomU32 random_number_generator;
		utility::forwardable_span<CountryInstance> countries_chunk;
		utility::forwardable_span<GoodInstance> goods_chunk;
		utility::forwardable_span<ProvinceInstance> provinces_chunk;

		constexpr WorkBundle() {}

		WorkBundle(
			RandomU32::state_type rng_state,
			utility::forwardable_span<CountryInstance> new_countries_chunk,
			utility::forwardable_span<GoodInstance> new_goods_chunk,
			utility::forwardable_span<ProvinceInstance> new_provinces_chunk
		) : random_number_generator { rng_state },
			countries_chunk { new_countries_chunk },
			goods_chunk { new_goods_chunk },
			provinces_chunk { new_provinces_chunk }
			{}
	};

	struct ThreadPool {
	private:
		enum struct work_t : uint8_t {
			NONE,
			GOOD_EXECUTE_ORDERS,
			PROVINCE_INITIALISE_FOR_NEW_GAME,
			PROVINCE_TICK,
			COUNTRY_TICK_BEFORE_MAP,
			COUNTRY_TICK_AFTER_MAP
		};

		constexpr static size_t WORK_BUNDLE_COUNT = 32;
		std::array<WorkBundle, WORK_BUNDLE_COUNT> all_work_bundles;
		memory::vector<std::thread> threads;
		memory::vector<work_t> work_per_thread;
		std::mutex thread_mutex, completed_mutex;
		std::condition_variable thread_condition, completed_condition;
		std::atomic<size_t> active_work_count = 0;
		bool is_cancellation_requested = false;
		Date const& current_date;
		bool is_good_execute_orders_requested = false;
		bool is_province_tick_requested = false;

		void loop_until_cancelled(
			work_t& work_type,
			GameRulesManager const& game_rules_manager,
			GoodInstanceManager const& good_instance_manager,
			ModifierEffectCache const& modifier_effect_cache,
			PopsDefines const& pop_defines,
			ProductionTypeManager const& production_type_manager,
			utility::forwardable_span<const CountryInstance> country_keys,
			utility::forwardable_span<const GoodDefinition> good_keys,
			utility::forwardable_span<const Strata> strata_keys,
			utility::forwardable_span<WorkBundle> work_bundles
		);
		void await_completion();
		void process_work(const work_t work_type);

	public:
		ThreadPool(Date const& new_current_date);
		~ThreadPool();

		void initialise_threadpool(
			GameRulesManager const& game_rules_manager,
			GoodInstanceManager const& good_instance_manager,
			ModifierEffectCache const& modifier_effect_cache,
			PopsDefines const& pop_defines,
			ProductionTypeManager const& production_type_manager,
			utility::forwardable_span<const GoodDefinition> good_keys,
			utility::forwardable_span<const Strata> strata_keys,			
			utility::forwardable_span<GoodInstance> goods,
			utility::forwardable_span<CountryInstance> countries,
			utility::forwardable_span<ProvinceInstance> provinces
		);

		void process_good_execute_orders();
		void process_province_ticks();
		void process_province_initialise_for_new_game();
		void process_country_ticks_before_map();
		void process_country_ticks_after_map();
	};
}