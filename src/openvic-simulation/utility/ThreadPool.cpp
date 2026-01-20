#include "ThreadPool.hpp"

#include "openvic-simulation/core/container/TypedSpan.hpp"
#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp" // IWYU pragma: keep for constructor requirement
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

void ThreadPool::loop_until_cancelled(
	work_t& work_type,
	GameRulesManager const& game_rules_manager,
	GoodInstanceManager const& good_instance_manager,
	ModifierEffectCache const& modifier_effect_cache,
	PopsDefines const& pop_defines,
	ProductionTypeManager const& production_type_manager,
	forwardable_span<const CountryInstance> country_keys,
	forwardable_span<const GoodDefinition> good_keys,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<WorkBundle> work_bundles
) {
	memory::IndexedFlatMap<GoodDefinition, char> reusable_goods_mask { good_keys };

	memory::FixedVector<fixed_point_t> reusable_country_map_0 { country_keys.size(), fixed_point_t::_0 };
	memory::FixedVector<fixed_point_t> reusable_country_map_1 { country_keys.size(), fixed_point_t::_0 };
	TypedSpan<country_index_t, fixed_point_t> reusable_country_map_0_span { reusable_country_map_0 };
	TypedSpan<country_index_t, fixed_point_t> reusable_country_map_1_span { reusable_country_map_1 };

	static constexpr size_t VECTOR_COUNT = std::max(
		GoodMarket::VECTORS_FOR_EXECUTE_ORDERS,
		std::max(
			CountryInstance::VECTORS_FOR_COUNTRY_TICK,
			ProvinceInstance::VECTORS_FOR_PROVINCE_TICK
		)
	);
	std::array<memory::vector<fixed_point_t>, VECTOR_COUNT> reusable_vectors;
	std::span<memory::vector<fixed_point_t>, VECTOR_COUNT> reusable_vectors_span = std::span(reusable_vectors);
	memory::vector<good_index_t> reusable_good_index_vector;
	PopValuesFromProvince reusable_pop_values {
		game_rules_manager,
		good_instance_manager,
		modifier_effect_cache,
		production_type_manager,
		pop_defines,
		strata_keys
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
				for (WorkBundle& work_bundle : work_bundles) {
					for (GoodMarket& good : work_bundle.goods_chunk) {
						good.execute_orders(
							reusable_country_map_0_span,
							reusable_country_map_1_span,
							reusable_vectors_span.first<GoodMarket::VECTORS_FOR_EXECUTE_ORDERS>()
						);
					}
				}
				break;
			case work_t::PROVINCE_TICK:
				for (WorkBundle& work_bundle : work_bundles) {
					for (ProvinceInstance& province : work_bundle.provinces_chunk) {
						province.province_tick(
							current_date,
							reusable_pop_values,
							work_bundle.random_number_generator,
							reusable_goods_mask,
							reusable_vectors_span.first<ProvinceInstance::VECTORS_FOR_PROVINCE_TICK>()
						);
					}
				}
				break;
			case work_t::PROVINCE_INITIALISE_FOR_NEW_GAME:
				for (WorkBundle& work_bundle : work_bundles) {
					for (ProvinceInstance& province : work_bundle.provinces_chunk) {
						province.initialise_for_new_game(
							current_date,
							reusable_pop_values,
							work_bundle.random_number_generator,
							reusable_goods_mask,
							reusable_vectors_span.first<ProvinceInstance::VECTORS_FOR_PROVINCE_TICK>()
						);
					}
				}
				break;
			case work_t::COUNTRY_TICK_BEFORE_MAP:
				for (WorkBundle& work_bundle : work_bundles) {
					for (CountryInstance& country : work_bundle.countries_chunk) {
						country.country_tick_before_map(
							reusable_goods_mask,
							reusable_vectors_span.first<CountryInstance::VECTORS_FOR_COUNTRY_TICK>(),
							reusable_good_index_vector
						);
					}
				}
				break;
			case work_t::COUNTRY_TICK_AFTER_MAP:
				for (WorkBundle& work_bundle : work_bundles) {
					for (CountryInstance& country : work_bundle.countries_chunk) {
						country.country_tick_after_map(current_date);
					}
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
	GameRulesManager const& game_rules_manager,
	GoodInstanceManager const& good_instance_manager,
	ModifierEffectCache const& modifier_effect_cache,
	PopsDefines const& pop_defines,
	ProductionTypeManager const& production_type_manager,
	forwardable_span<const GoodDefinition> good_keys,
	forwardable_span<const Strata> strata_keys,
	forwardable_span<GoodInstance> goods,
	forwardable_span<CountryInstance> countries,
	forwardable_span<ProvinceInstance> provinces
) {
	if (threads.size() > 0) {
		spdlog::error_s("Attempted to initialise ThreadPool again.");
		return;
	}

	RandomU32 master_rng { }; //TODO seed?


	const auto [goods_quotient, goods_remainder] = std::ldiv(goods.size(), WORK_BUNDLE_COUNT);
	const auto [countries_quotient, countries_remainder] = std::ldiv(countries.size(), WORK_BUNDLE_COUNT);
	const auto [provinces_quotient, provinces_remainder] = std::ldiv(provinces.size(), WORK_BUNDLE_COUNT);
	auto goods_begin = goods.begin();
	auto countries_begin = countries.begin();
	auto provinces_begin = provinces.begin();

	for (size_t i = 0; i < WORK_BUNDLE_COUNT; i++) {
		const size_t goods_chunk_size = i < goods_remainder
			? goods_quotient + 1
			: goods_quotient;
		const size_t countries_chunk_size = i < countries_remainder
			? countries_quotient + 1
			: countries_quotient;
		const size_t provinces_chunk_size = i < provinces_remainder
			? provinces_quotient + 1
			: provinces_quotient;

		auto goods_end = goods_begin + goods_chunk_size;
		auto countries_end = countries_begin + countries_chunk_size;
		auto provinces_end = provinces_begin + provinces_chunk_size;

		all_work_bundles[i] = WorkBundle {
			master_rng.generator().serialize(),
			std::span<CountryInstance>{ countries_begin, countries_end },
			std::span<GoodInstance>{ goods_begin, goods_end },
			std::span<ProvinceInstance>{ provinces_begin, provinces_end }
		};

		//ensure different state for next WorkBundle
		master_rng.generator().jump();

		goods_begin = goods_end;
		countries_begin = countries_end;
		provinces_begin = provinces_end;
	}

	const size_t max_worker_threads = std::max<size_t>(std::thread::hardware_concurrency(), 1);
	threads.reserve(max_worker_threads);
	work_per_thread.resize(max_worker_threads, work_t::NONE);

	
	const auto [work_bundles_quotient, work_bundles_remainder] = std::ldiv(WORK_BUNDLE_COUNT, max_worker_threads);
	auto work_bundles_begin = all_work_bundles.begin();

	for (size_t i = 0; i < max_worker_threads; ++i) {
		const size_t work_bundles_chunk_size = i < work_bundles_remainder
			? work_bundles_quotient + 1
			: work_bundles_quotient;

		auto work_bundles_end = work_bundles_begin + work_bundles_chunk_size;

		threads.emplace_back(
			[
				this,
				&work_for_thread = work_per_thread[i],
				&game_rules_manager,
				&good_instance_manager,
				&modifier_effect_cache,
				&pop_defines,
				&production_type_manager,
				countries,
				good_keys,
				strata_keys,
				work_bundles_begin,
				work_bundles_end
			]() -> void {
				loop_until_cancelled(
					work_for_thread,
					game_rules_manager,
					good_instance_manager,
					modifier_effect_cache,
					pop_defines,
					production_type_manager,
					countries,
					good_keys,
					strata_keys,
					std::span<WorkBundle>{ work_bundles_begin, work_bundles_end }
				);
			}
		);

		work_bundles_begin = work_bundles_end;
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

void ThreadPool::process_country_ticks_before_map() {
	process_work(work_t::COUNTRY_TICK_BEFORE_MAP);
}

void ThreadPool::process_country_ticks_after_map(){
	process_work(work_t::COUNTRY_TICK_AFTER_MAP);
}