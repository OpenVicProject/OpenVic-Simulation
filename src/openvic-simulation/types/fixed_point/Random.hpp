#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct uniform_real_distribution {
	public:
		using result_type = fixed_point_t;
	private:
		using raw_result_type = fixed_point_t::value_type;
		const result_type _min;
		const result_type _max;
	public:
		result_type min() const {
			return _min;
		}
		result_type max() const {
			return _max;
		}

		constexpr uniform_real_distribution() : uniform_real_distribution(fixed_point_t::_0) {}
		constexpr explicit uniform_real_distribution(
			result_type new_min,
			result_type new_max = fixed_point_t::usable_max
		) : _min { new_min },
			_max { new_max } {}

		template<typename T>
		requires requires(T& g) { { g() } -> std::integral; }
		&& requires { { T::min() } -> std::integral; }
		&& requires { { T::max() } -> std::integral; }
		[[nodiscard]] result_type operator() (T& generator) {
			const auto raw_value = generator();
			constexpr const auto generator_min = T::min();
			constexpr const auto generator_max = T::max();
			
			const auto difference = raw_value - generator_min;
			const auto generator_range = generator_max - generator_min;

			const result_type range_size = _max - _min;
			return _min + range_size * difference / generator_range;
        }
	};
}