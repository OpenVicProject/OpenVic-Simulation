#pragma once

#include <atomic>
#include <thread>

namespace OpenVic {
	struct spin_mutex {
		spin_mutex() = default;
		~spin_mutex() = default;
		spin_mutex(spin_mutex const&) = delete;
		spin_mutex& operator=(spin_mutex const&) = delete;
		spin_mutex(spin_mutex&&) = delete;
		spin_mutex& operator=(spin_mutex&&) = delete;

		void lock() {
			while (true) {
				while (!state.load(std::memory_order_relaxed)) {
					std::this_thread::yield();
				}

				if (try_lock()) {
					break;
				}
			}
		}

		bool try_lock() {
			return state.exchange(false, std::memory_order_acquire);
		}

		void unlock() {
			state.store(true, std::memory_order_release);
		}

	private:
		std::atomic_bool state = true;
	};
}
