#pragma once

#include "FixedPoint.hpp"

#include <cassert>

#include "openvic-simulation/core/MathSqrt.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

/* Sin lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_sin.hpp"

namespace OpenVic::fp {
	static_assert(_detail::LUT::SIN_PRECISION == fixed_point_t::PRECISION);	

	OV_SPEED_INLINE static constexpr fixed_point_t sin(fixed_point_t const& v) {
		using namespace _detail::LUT;

		fixed_point_t::value_type num = (v % fixed_point_t::pi2 * fixed_point_t::one_div_pi2).get_raw_value();

		const bool negative = num < 0;
		if (negative) {
			num = -num;
		}

		const fixed_point_t::value_type index = num >> SIN_SHIFT;
		const fixed_point_t::value_type a = SIN[index];
		const fixed_point_t::value_type b = SIN[index + 1];

		const fixed_point_t::value_type fraction = (num - (index << SIN_SHIFT)) << SIN_COUNT_LOG2;
		const fixed_point_t::value_type result = a + (((b - a) * fraction) >> SIN_PRECISION);
		return fixed_point_t::parse_raw(
			!negative ? result : -result
		);
	}

	OV_SPEED_INLINE static constexpr fixed_point_t cos(fixed_point_t const& v) {
		return fp::sin(v + fixed_point_t::pi_half);
	}

	OV_SPEED_INLINE static constexpr fixed_point_t sqrt(fixed_point_t const& v) {
		return !v.is_negative()
			? fixed_point_t::parse_raw(
				OpenVic::sqrt(
					static_cast<uint64_t>(v.get_raw_value()) << fixed_point_t::PRECISION
				)
			)
			: fixed_point_t::_0;
	}

	//Preserves accuracy. Performing a normal multiplication of small values results in 0.
	template<std::integral T>
	OV_SPEED_INLINE static constexpr fixed_point_t mul_div(fixed_point_t const& multiplier, T const& numerator, T const& denominator) {
		return fixed_point_t::parse_raw(multiplier.get_raw_value() * numerator / denominator);
	}

	OV_SPEED_INLINE static constexpr fixed_point_t mul_div(fixed_point_t const& multiplier, fixed_point_t const& numerator, fixed_point_t const& denominator) {
		return mul_div(multiplier, numerator.get_raw_value(), denominator.get_raw_value());
	}

	template<is_strongly_typed StrongType>
	OV_SPEED_INLINE static constexpr fixed_point_t mul_div(fixed_point_t const& multiplier, StrongType const& numerator, StrongType const& denominator) {
		return mul_div(multiplier, type_safe::get(numerator), type_safe::get(denominator));
	}
	
	template<std::integral T>
	OV_SPEED_INLINE static constexpr T multiply_truncate(T const& integer, fixed_point_t const& v) {
		return static_cast<T>(
			(static_cast<fixed_point_t::value_type>(integer) * v.get_raw_value()) >> fixed_point_t::PRECISION
		);
	}

	template<is_strongly_typed StrongType>
	OV_SPEED_INLINE static constexpr StrongType multiply_truncate(StrongType const& integer, fixed_point_t const& v) {
		return StrongType(multiply_truncate(
			type_safe::get(integer),
			v
		));
	}

	template<integral_max_size_4 TNumerator, integral_max_size_4 TDenominator>
	OV_SPEED_INLINE static constexpr fixed_point_t from_fraction(const TNumerator numerator, const TDenominator denominator) {
		return fixed_point_t::parse_raw(
			(static_cast<fixed_point_t::value_type>(numerator) << fixed_point_t::PRECISION)
			/ static_cast<fixed_point_t::value_type>(denominator)
		);
	}

	OV_SPEED_INLINE static constexpr fixed_point_t from_fraction(const int64_t numerator, const int64_t denominator) {
		assert(numerator < 140737488355328LL); //2^47
		assert(numerator >= -140737488355328LL); //- 2^47
		return fixed_point_t::parse_raw((numerator << fixed_point_t::PRECISION) / denominator);
	}

	OV_SPEED_INLINE static constexpr fixed_point_t from_fraction(const uint64_t numerator, const uint64_t denominator) {
		assert(numerator < 281474976710656ULL); //2^48
		return fixed_point_t::parse_raw((numerator << fixed_point_t::PRECISION) / denominator);
	}

	template<is_strongly_typed StrongType>
	OV_SPEED_INLINE static constexpr fixed_point_t from_fraction(StrongType const& numerator, StrongType const& denominator) {
		return from_fraction(type_safe::get(numerator), type_safe::get(denominator));
	}
}