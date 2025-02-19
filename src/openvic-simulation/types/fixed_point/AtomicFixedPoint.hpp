#pragma once

#include <atomic>
#include <memory>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct atomic_fixed_point_t {
	private:
		std::unique_ptr<std::atomic<decltype(fixed_point_t::value)>> raw_value_ptr;

	public:
		atomic_fixed_point_t(fixed_point_t value = fixed_point_t::_0())
			: raw_value_ptr { std::make_unique<std::atomic<decltype(fixed_point_t::value)>>(value.get_raw_value()) }
			{}

		fixed_point_t get_value() const {
			return fixed_point_t::parse_raw(*raw_value_ptr);
		}

		fixed_point_t set_value(fixed_point_t const& rhs) {
			return *raw_value_ptr = rhs.get_raw_value();
		}

		atomic_fixed_point_t operator+=(fixed_point_t const& rhs) {
			return { fixed_point_t::parse_raw(*raw_value_ptr += rhs.get_raw_value()) };
		}

		atomic_fixed_point_t operator-=(fixed_point_t const& rhs) {
			return { fixed_point_t::parse_raw(*raw_value_ptr -= rhs.get_raw_value()) };
		}
	};
}