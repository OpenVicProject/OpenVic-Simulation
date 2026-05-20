#include "ThreadPool.hpp"
#include "ThreadDeps.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <thread>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp" // IWYU pragma: keep for constructor requirement
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;

static constexpr std::size_t VECTOR_COUNT = std::max(
	GoodMarket::VECTORS_FOR_EXECUTE_ORDERS,
	std::max(
		CountryInstance::VECTORS_FOR_COUNTRY_TICK,
		ProvinceInstance::VECTORS_FOR_PROVINCE_TICK
	)
);

struct SectionContext {
public:
	Date const& current_date;
	std::span<WorkBundle> work_bundles;
	MapInstance const& map_instance;
	MilitaryDefines const& military_defines;
	StaticModifierCache const& static_modifier_cache;
	memory::FixedVector<char, good_index_t> reusable_goods_mask;
	memory::FixedVector<fixed_point_t, country_index_t> reusable_country_map_0;
	memory::FixedVector<fixed_point_t, country_index_t> reusable_country_map_1;
private:
	std::array<memory::vector<fixed_point_t>, VECTOR_COUNT> _reusable_vectors;
public:
	std::span<memory::vector<fixed_point_t>, VECTOR_COUNT> reusable_vectors { _reusable_vectors };
	memory::vector<good_index_t> reusable_good_index_vector;
	PopValuesFromProvince reusable_pop_values;

	constexpr SectionContext(
		Date const& current_date_ref,
		std::span<WorkBundle> new_work_bundles,
		ThreadDeps const& deps
	) : current_date { current_date_ref },
		work_bundles { new_work_bundles },
		map_instance { deps.map_instance },
		military_defines { deps.military_defines },
		static_modifier_cache { deps.static_modifier_cache },
		reusable_goods_mask { deps.good_count, {} },
		reusable_country_map_0 { deps.country_count, fixed_point_t::_0 },
		reusable_country_map_1 { deps.country_count, fixed_point_t::_0 },
		reusable_pop_values {
			deps.game_rules_manager,
			deps.good_instance_manager,
			deps.modifier_effect_cache,
			deps.production_type_manager,
			deps.pop_defines,
			deps.strata_count
		}
		{}
};

std::optional<SectionContext> main_thread_ctx;

static void process_section(const work_t work_type, SectionContext& ctx) {
	for (WorkBundle& work_bundle : ctx.work_bundles) {
		switch (work_type) {
			case work_t::NONE:
				break;
			case work_t::GOOD_EXECUTE_ORDERS:
				for (GoodMarket& good : work_bundle.goods_chunk) {
					good.execute_orders(
						ctx.reusable_country_map_0,
						ctx.reusable_country_map_1,
						ctx.reusable_vectors.first<GoodMarket::VECTORS_FOR_EXECUTE_ORDERS>()
					);
				}
				break;
			case work_t::PROVINCE_TICK:
				for (ProvinceInstance& province : work_bundle.provinces_chunk) {
					province.province_tick(
						ctx.current_date,
						ctx.reusable_pop_values,
						work_bundle.random_number_generator,
						ctx.reusable_goods_mask,
						ctx.reusable_vectors.first<ProvinceInstance::VECTORS_FOR_PROVINCE_TICK>()
					);
				}
				break;
			case work_t::PROVINCE_INITIALISE_FOR_NEW_GAME:
				for (ProvinceInstance& province : work_bundle.provinces_chunk) {
					province.initialise_for_new_game(
						ctx.current_date,
						ctx.map_instance,
						ctx.reusable_pop_values,
						work_bundle.random_number_generator,
						ctx.reusable_goods_mask,
						ctx.reusable_vectors.first<ProvinceInstance::VECTORS_FOR_PROVINCE_TICK>()
					);
				}
				break;
			case work_t::COUNTRY_TICK_BEFORE_MAP:
				for (CountryInstance& country : work_bundle.countries_chunk) {
					country.country_tick_before_map(
						ctx.reusable_goods_mask,
						ctx.reusable_vectors.first<CountryInstance::VECTORS_FOR_COUNTRY_TICK>(),
						ctx.reusable_good_index_vector
					);
				}
				break;
			case work_t::COUNTRY_TICK_AFTER_MAP:
				for (CountryInstance& country : work_bundle.countries_chunk) {
					country.country_tick_after_map(ctx.current_date);
				}
				break;
			case work_t::PROVINCE_UPDATE_GAMESTATE:
				for (ProvinceInstance& province : work_bundle.provinces_chunk) {
					province.update_gamestate(
						ctx.current_date,
						ctx.military_defines
					);
				}
				break;
			case work_t::COUNTRY_UPDATE_GAMESTATE_AFTER_MAP:
				for (CountryInstance& country : work_bundle.countries_chunk) {
					country.update_gamestate_after_map(ctx.current_date);
				}
				break;
			case work_t::PROVINCE_UPDATE_MODIFIER_SUMS:
				for (ProvinceInstance& province : work_bundle.provinces_chunk) {
					province.update_modifier_sum(
						ctx.current_date,
						ctx.static_modifier_cache
					);
				}
				break;
			case work_t::COUNTRY_UPDATE_MODIFIER_SUMS_BEFORE_MAP:
				for (CountryInstance& country : work_bundle.countries_chunk) {
					country.update_modifier_sum_before_map(
						ctx.current_date,
						ctx.static_modifier_cache
					);
				}
				break;
			case work_t::COUNTRY_UPDATE_MODIFIER_SUMS_AFTER_MAP:
				for (CountryInstance& country : work_bundle.countries_chunk) {
					country.update_modifier_sum_after_map(ctx.current_date);
				}
				break;
		}
	}
}

void ThreadPool::loop_until_cancelled(
	work_t& work_type,
	const ThreadDeps deps,
	forwardable_span<WorkBundle> work_bundles
) {
	SectionContext thread_ctx { current_date, work_bundles, deps };

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

		process_section(work_type_copy, thread_ctx);

		{
			std::lock_guard<std::mutex> completed_lock { completed_mutex };
			if (--active_work_count == 0) {
				completed_condition.notify_one();
			}
		}
	}
}

void ThreadPool::process(const work_t work_type) {
	if (!main_thread_ctx.has_value()) {
		spdlog::error_s("ThreadPool isn't initialised.");
		return;
	}

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
	process_section(work_type, main_thread_ctx.value());
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
	ThreadDeps const& deps,
	forwardable_span<GoodInstance> goods,
	forwardable_span<CountryInstance> countries,
	forwardable_span<ProvinceInstance> provinces
) {
	if (main_thread_ctx.has_value()) {
		spdlog::error_s("Attempted to initialise ThreadPool again.");
		return;
	}

	RandomU32 master_rng { }; //TODO seed?


	const auto [goods_quotient, goods_remainder] = std::ldiv(goods.size(), WORK_BUNDLE_COUNT);
	const auto [countries_quotient, countries_remainder] = std::ldiv(countries.size(),WORK_BUNDLE_COUNT);
	const auto [provinces_quotient, provinces_remainder] = std::ldiv(provinces.size(),WORK_BUNDLE_COUNT);
	auto goods_begin = goods.begin();
	auto countries_begin = countries.begin();
	auto provinces_begin = provinces.begin();

	for (std::size_t i = 0; i < WORK_BUNDLE_COUNT; i++) {
		const std::size_t goods_chunk_size = i < goods_remainder
			? goods_quotient + 1
			: goods_quotient;
		const std::size_t countries_chunk_size = i < countries_remainder
			? countries_quotient + 1
			: countries_quotient;
		const std::size_t provinces_chunk_size = i < provinces_remainder
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

	const std::size_t max_worker_threads = std::min(
		std::max<std::size_t>(1, std::thread::hardware_concurrency()),
		WORK_BUNDLE_COUNT
	);
	
	const auto [
		work_bundles_quotient,
		work_bundles_remainder
	] = std::ldiv(
		WORK_BUNDLE_COUNT,
		max_worker_threads
	);
	
	const std::size_t extra_worker_thread_count = max_worker_threads - 1; // main thread also works
	threads.reserve(extra_worker_thread_count);
	work_per_thread.resize(extra_worker_thread_count, work_t::NONE);

	auto work_bundles_begin = all_work_bundles.begin();
	for (std::size_t i = 0; i < max_worker_threads; ++i) {
		const std::size_t work_bundles_chunk_size = i < work_bundles_remainder
			? work_bundles_quotient + 1
			: work_bundles_quotient;

		auto work_bundles_end = work_bundles_begin + work_bundles_chunk_size;
		std::span<WorkBundle> work_bundles { work_bundles_begin, work_bundles_end };
		if (i < extra_worker_thread_count) {
			threads.emplace_back(
				[
					this,
					&work_for_thread = work_per_thread[i],
					deps,
					work_bundles
				]() -> void {
					loop_until_cancelled(
						work_for_thread,
						deps,
						work_bundles
					);
				}
			);
		} else {
			main_thread_ctx.emplace(
				current_date,
				work_bundles,
				deps
			);
		}

		work_bundles_begin = work_bundles_end;
	}
}