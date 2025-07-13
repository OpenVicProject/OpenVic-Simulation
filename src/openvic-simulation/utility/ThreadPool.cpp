#include "ThreadPool.hpp"

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

void ThreadPool::loop_until_cancelled(
	work_t& work_type,
	PopsDefines const& pop_defines,
	utility::forwardable_span<const CountryInstance> country_keys,
	utility::forwardable_span<const Strata> strata_keys,
	utility::forwardable_span<const GoodDefinition> good_definition_keys,
	std::span<GoodInstance> goods_chunk,
	std::span<ProvinceInstance> provinces_chunk
) {
	IndexedMap<CountryInstance, fixed_point_t> reusable_country_map_0 { country_keys },
		reusable_country_map_1 { country_keys };
	memory::vector<fixed_point_t> reusable_vector_0 {}, reusable_vector_1 {};
	PopValuesFromProvince reusable_pop_values { 
		pop_defines,
		strata_keys,
		good_definition_keys
	};

	while (!is_cancellation_requested) {
		work_t work_type_copy;
		{
			std::unique_lock<std::mutex> thread_lock { thread_mutex };
			thread_condition.wait(
				thread_lock,
				[this, &work_type]() -> bool {
					return is_cancellation_requested || work_type != work_t::NONE;
				}
			);

			if (is_cancellation_requested) {
				return;
			}
			work_type_copy = work_type;
			work_type = work_t::NONE;
		}

		switch (work_type_copy) {
			case work_t::NONE:
				break;
			case work_t::GOOD_EXECUTE_ORDERS:
				for (GoodMarket& good : goods_chunk) {
					good.execute_orders(
						reusable_country_map_0,
						reusable_country_map_1,
						reusable_vector_0,
						reusable_vector_1
					);
				}
				break;
			case work_t::PROVINCE_TICK:
				for (ProvinceInstance& province : provinces_chunk) {
					province.province_tick(current_date, reusable_pop_values, reusable_vector_0);
				}
				break;
			case work_t::PROVINCE_INITIALISE_FOR_NEW_GAME:
				for (ProvinceInstance& province : provinces_chunk) {
					province.initialise_for_new_game(current_date, reusable_pop_values, reusable_vector_0);
				}
				break;
		}

		{
			std::lock_guard<std::mutex> completed_lock { completed_mutex };
			if (--active_work_count == 0) {
				completed_condition.notify_one();
			}
		}
	}
}

void ThreadPool::process_work(const work_t work_type) {
	{
		std::unique_lock<std::mutex> thread_lock { thread_mutex };
		if (is_cancellation_requested) {
			return;
		}

		{
			std::lock_guard<std::mutex> completed_lock { completed_mutex };
			active_work_count = threads.size();
		}

		for (work_t& work_for_thread : work_per_thread) {
			work_for_thread = work_type;
		}
		thread_condition.notify_all();
	}
	await_completion();
}

void ThreadPool::await_completion() {
	std::unique_lock<std::mutex> completed_lock { completed_mutex };
	completed_condition.wait(
		completed_lock,
		[this]() -> bool {
			return active_work_count == 0;
		}
	);
}

ThreadPool::ThreadPool(Date const& new_current_date)
	: current_date {new_current_date} {}

ThreadPool::~ThreadPool() {
	{
		std::lock_guard<std::mutex> thread_lock { thread_mutex };
		is_cancellation_requested = true;
		thread_condition.notify_all();
	}
	for (std::thread& thread : threads) {
		thread.join();
	}
}

void ThreadPool::initialise_threadpool(
	PopsDefines const& pop_defines,
	utility::forwardable_span<const CountryInstance> country_keys,
	utility::forwardable_span<const Strata> strata_keys,
	utility::forwardable_span<const GoodDefinition> good_definition_keys,
	std::span<GoodInstance> goods,
	std::span<ProvinceInstance> provinces
) {
	if (threads.size() > 0) {
		Logger::error("Attempted to initialise ThreadPool again.");
		return;
	}

	const size_t max_worker_threads = std::max<size_t>(std::thread::hardware_concurrency(), 1);
	threads.reserve(max_worker_threads);
	work_per_thread.resize(max_worker_threads, work_t::NONE);

	const auto [goods_quotient, goods_remainder] = std::ldiv(goods.size(), max_worker_threads);
	const auto [provinces_quotient, provinces_remainder] = std::ldiv(provinces.size(), max_worker_threads);

	typename std::span<GoodInstance>::iterator goods_begin = goods.begin();
	typename std::span<ProvinceInstance>::iterator provinces_begin = provinces.begin();

	for (size_t i = 0; i < max_worker_threads; i++) {
		const size_t goods_chunk_size = i < goods_remainder
			? goods_quotient + 1
			: goods_quotient;
		const size_t provinces_chunk_size = i < provinces_remainder
			? provinces_quotient + 1
			: provinces_quotient;

		typename std::span<GoodInstance>::iterator goods_end = goods_begin + goods_chunk_size;
		typename std::span<ProvinceInstance>::iterator provinces_end = provinces_begin + provinces_chunk_size;

		threads.emplace_back(
			[
				this,
				&work_for_thread = work_per_thread[i],
				&pop_defines,
				country_keys,
				strata_keys,
				good_definition_keys,
				goods_begin, goods_end,
				provinces_begin, provinces_end
			]() -> void {
				loop_until_cancelled(
					work_for_thread,
					pop_defines,
					country_keys,
					strata_keys,
					good_definition_keys,
					std::span<GoodInstance>{ goods_begin, goods_end },
					std::span<ProvinceInstance>{ provinces_begin, provinces_end }
				);
			}
		);

		goods_begin = goods_end;
		provinces_begin = provinces_end;
	}
}

void ThreadPool::process_good_execute_orders() {
	process_work(work_t::GOOD_EXECUTE_ORDERS);
}

void ThreadPool::process_province_ticks() {
	process_work(work_t::PROVINCE_TICK);
}

void ThreadPool::process_province_initialise_for_new_game() {
	process_work(work_t::PROVINCE_INITIALISE_FOR_NEW_GAME);
}