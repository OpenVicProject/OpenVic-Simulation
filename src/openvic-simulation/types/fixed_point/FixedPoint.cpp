#include "FixedPoint.hpp"

#include <concepts>
#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/utility/Logger.hpp"

#include "String.hpp"

/* Base e exponential lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_2_16_EXP_e.hpp"

/* Base 2001 exponential lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_2_16_EXP_2001.hpp"

using namespace OpenVic;
static_assert(_detail::LUT::_2_16_EXP_e_DIVISOR == 1 << fixed_point_t::PRECISION);
static_assert(_detail::LUT::_2_16_EXP_2001_DIVISOR == 1 << fixed_point_t::PRECISION);

OV_SPEED_INLINE void fixed_point_t::warn_if_truncated() const {
	if (OV_unlikely(
		value != 0
		&& abs() < fixed_point_t::_1
	)) {
		spdlog::warn_s(
			"0 < abs(Fixed point) < 1, truncation will result in zero, this may be a bug. raw_value: {} as float: {}",
			get_raw_value(),
			static_cast<float>(*this)
		);
	}
}

fmt::format_context::iterator fmt::formatter<fixed_point_t>::format(fixed_point_t fp, format_context& ctx) const {
	format_specs specs { _specs };
	if (_specs.dynamic()) {
		detail::handle_dynamic_spec(_specs.dynamic_precision(), specs.precision, _specs.precision_ref, ctx);
		detail::handle_dynamic_spec(_specs.dynamic_width(), specs.width, _specs.width_ref, ctx);
	}

	fp::stack_string result = fp::to_array(fp, specs.precision);
	if (OV_unlikely(result.empty())) {
		return ctx.out();
	}

	string_view view { result.data(), result.size() };

	sign s = fp.is_negative() ? sign::minus : _specs.sign();

	format_context::iterator out = ctx.out();
	if (_specs.align() == align::numeric && s != sign::none) {
		if (s == sign::minus) {
			view = { view.data() + 1, view.size() - 1 };
		}

		*out++ = detail::getsign<char>(s);
		s = sign::none;
		if (specs.width != 0) {
			--specs.width;
		}
	}
	// Prevents string precision behavior
	specs.precision = -1;

	if (s == sign::none || s == sign::minus) {
		return detail::write<char>(out, view, specs, ctx.locale());
	}

	memory_buffer buffer;
	buffer.resize(view.size() + 1);

	decltype(buffer)::value_type* buf_out = buffer.data();
	*buf_out++ = detail::getsign<char>(s);
	buf_out = detail::write(buf_out, view);

	view = { buffer.data(), buffer.size() };
	return detail::write<char>(out, view, specs, ctx.locale());
}

namespace OpenVic {
	std::ostream& operator<<(std::ostream& stream, fixed_point_t const& obj) {
		fp::stack_string result = fp::to_array(obj);
		if (OV_unlikely(result.empty())) {
			return stream;
		}

		return stream << static_cast<std::string_view>(result);
	}
}

template<std::integral T>
requires (sizeof(T) >= 4)
static fixed_point_t parse_capped_generic(const T value) {
	fixed_point_t result;
	if (value > std::numeric_limits<int32_t>::max()) {
		if (value >= fixed_point_t::max.truncate<T>()) {
			spdlog::error_s("parse_capped value exceeded int32 max. Falling back to fixed_point_t::max");
			result = fixed_point_t::max;
		} else {
			spdlog::warn_s("parse_capped value exceeded int32 max. It still fits but exceeds fixed_point_t::usable_max");
			result = fixed_point_t::parse_raw(value << fixed_point_t::PRECISION);
		}
	} else {
		result = fixed_point_t(static_cast<int32_t>(value));
	}

	return result;
}

fixed_point_t fixed_point_t::parse_capped(const int64_t value) { return parse_capped_generic(value); }
fixed_point_t fixed_point_t::parse_capped(const uint64_t value) { return parse_capped_generic(value); }

template<size_t N, std::array<int64_t, N> EXP_LUT>
OV_SPEED_INLINE static constexpr fixed_point_t _exp_internal(fixed_point_t const& x) {
	const bool negative = x.is_negative();
	fixed_point_t::value_type bits = negative ? -x.get_raw_value() : x.get_raw_value();
	fixed_point_t result = fixed_point_t::_1;

	for (size_t index = 0; bits != 0 && index < EXP_LUT.size(); ++index, bits >>= 1) {
		if (bits & 1LL) {
			result *= fixed_point_t::parse_raw(EXP_LUT[index]);
		}
	}

	if (bits != 0) {
		spdlog::error_s("Fixed point exponential overflow!");
	}

	if (negative) {
		return fixed_point_t::_1 / result;
	} else {
		return result;
	}
}

fixed_point_t fixed_point_t::exp(fixed_point_t const& x) {
	return _exp_internal<_detail::LUT::_2_16_EXP_e.size(), _detail::LUT::_2_16_EXP_e>(x);
}

fixed_point_t fixed_point_t::exp_2001(fixed_point_t const& x) {
	return _exp_internal<_detail::LUT::_2_16_EXP_2001.size(), _detail::LUT::_2_16_EXP_2001>(x);
}
