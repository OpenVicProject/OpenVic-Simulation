#pragma once

#include <thread>
#include <utility>
#include <vector>

namespace OpenVic {
	template<typename T_SHARED_VALUE>
	struct ParallelProcessorWithSharesValues {
	private:
		std::vector<T_SHARED_VALUE> reusable_shared_values_collection;
		std::vector<std::thread> threads;

	public:
		ParallelProcessorWithSharesValues() = default;
		ParallelProcessorWithSharesValues(ParallelProcessorWithSharesValues&&) = default;

		template <typename... Args>
		void init_parallel_processor(Args&&... args) {
			const size_t max_worker_threads = std::thread::hardware_concurrency();
			const size_t max_threads_including_parent = max_worker_threads + 1;
			reusable_shared_values_collection.reserve(max_threads_including_parent);
			threads.reserve(max_worker_threads);

			for (size_t i = 0; i < max_worker_threads; i++) {
				reusable_shared_values_collection.emplace_back(std::forward<Args>(args)...);
				threads.emplace_back();
			}

			reusable_shared_values_collection.emplace_back(std::forward<Args>(args)...);
		}

		template<typename T_ITEM>
		void process_in_parallel(std::vector<T_ITEM>& items, std::invocable<T_ITEM&, T_SHARED_VALUE&> auto callback) {
			const auto [quotient, remainder] = std::ldiv(items.size(), reusable_shared_values_collection.size());
			typename std::vector<T_ITEM>::iterator begin = items.begin();
			for (size_t i = 0; i < threads.size(); i++) {
				const size_t chunk_size = i < remainder
					? quotient + 1
					: quotient;
				typename std::vector<T_ITEM>::iterator end = begin + chunk_size;
				threads[i] = std::thread{
					[
						&callback,
						&reusable_shared_values = reusable_shared_values_collection[i],
						begin,
						end
					]()->void{
						for (typename std::vector<T_ITEM>::iterator it = begin; it < end; it++) {
							callback(*it, reusable_shared_values);
						}
					}
				};
				begin = end;
			}
			{
				typename std::vector<T_ITEM>::iterator parent_thread_end = begin + quotient;
				T_SHARED_VALUE& reusable_shared_values = reusable_shared_values_collection.back();
				for (typename std::vector<T_ITEM>::iterator it = begin; it < parent_thread_end; it++) {
					callback(*it, reusable_shared_values);
				}
			}

			for (std::thread& thread : threads) {
				if (thread.joinable()) {
					thread.join();
				}
			}
		}
	};
}