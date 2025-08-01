#pragma once

namespace OpenVic {
	struct null_mutex {
		constexpr null_mutex() {};
		~null_mutex() = default;
		null_mutex(null_mutex const&) = delete;
		null_mutex& operator=(null_mutex const&) = delete;
		null_mutex(null_mutex&&) = delete;
		null_mutex& operator=(null_mutex&&) = delete;

		inline bool try_lock() {
			return true;
		}
		inline void lock() {}
		inline void unlock() {}
	};
}