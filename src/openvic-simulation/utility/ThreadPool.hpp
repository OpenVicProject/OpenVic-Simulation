#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct GoodDefinition;
	struct CountryInstance;
	struct GoodInstance;
	struct PopsDefines;
	struct Strata;

	struct ThreadPool {
	private:
		enum struct work_t : uint8_t {
			NONE,
			GOOD_EXECUTE_ORDERS,
			PROVINCE_INITIALISE_FOR_NEW_GAME,
			PROVINCE_TICK
		};

		void loop_until_cancelled(
			work_t& work_type,
			PopsDefines const& pop_defines,
			utility::forwardable_span<const CountryInstance> country_keys,
			utility::forwardable_span<const GoodDefinition> good_keys,
			utility::forwardable_span<const Strata> strata_keys,
			utility::forwardable_span<GoodInstance> goods_chunk,
			utility::forwardable_span<ProvinceInstance> provinces_chunk
		);

		memory::vector<std::thread> threads;
		memory::vector<work_t> work_per_thread;
		std::mutex thread_mutex, completed_mutex;
		std::condition_variable thread_condition, completed_condition;
		std::atomic<size_t> active_work_count = 0;
		bool is_cancellation_requested = false;
		Date const& current_date;
		bool is_good_execute_orders_requested = false;
		bool is_province_tick_requested = false;

		void await_completion();
		void process_work(const work_t work_type);

	public:
		ThreadPool(Date const& new_current_date);
		~ThreadPool();

		void initialise_threadpool(
			PopsDefines const& pop_defines,
			utility::forwardable_span<const CountryInstance> country_keys,
			utility::forwardable_span<const GoodDefinition> good_keys,
			utility::forwardable_span<const Strata> strata_keys,			
			utility::forwardable_span<GoodInstance> goods,
			utility::forwardable_span<ProvinceInstance> provinces
		);

		void process_good_execute_orders();
		void process_province_ticks();
		void process_province_initialise_for_new_game();
	};
}