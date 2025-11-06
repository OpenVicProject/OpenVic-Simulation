#pragma once

#include "openvic-simulation/utility/Typedefs.hpp"

namespace OpenVic {
	struct null_mutex {
		OV_ALWAYS_INLINE constexpr null_mutex() {};
		OV_ALWAYS_INLINE ~null_mutex() = default;
		OV_ALWAYS_INLINE null_mutex(null_mutex const&) = delete;
		OV_ALWAYS_INLINE null_mutex& operator=(null_mutex const&) = delete;
		OV_ALWAYS_INLINE null_mutex(null_mutex&&) = delete;
		OV_ALWAYS_INLINE null_mutex& operator=(null_mutex&&) = delete;

		OV_ALWAYS_INLINE bool try_lock() {
			return true;
		}
		OV_ALWAYS_INLINE void lock() {}
		OV_ALWAYS_INLINE void unlock() {}
	};
}
