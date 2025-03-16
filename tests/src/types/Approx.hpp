#pragma once

#include <compare>
#include <concepts>
#include <cstdlib>
#include <limits>

#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/Utility.hpp"

#include "Vector.hpp" // IWYU pragma: keep
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace OpenVic::testing {
	struct approx {
		constexpr approx(double value)
			: _epsilon(static_cast<double>(std::numeric_limits<float>::epsilon()) * 100), //
			  _scale(1.0), //
			  _value(value) {}

		constexpr approx operator()(double value) const {
			approx approx(value);
			approx.epsilon(_epsilon);
			approx.scale(_scale);
			return approx;
		}

		template<std::convertible_to<double> T>
		explicit constexpr approx(T const& value) { // NOLINT(cppcoreguidelines-pro-type-member-init)
			*this = static_cast<double>(value);
		}

		constexpr approx& epsilon(double epsilon) {
			_epsilon = epsilon;
			return *this;
		}

		template<std::convertible_to<double> T>
		constexpr approx& epsilon(T const& psilon) {
			_epsilon = static_cast<double>(psilon);
			return *this;
		}

		constexpr approx& scale(double scale) {
			_scale = scale;
			return *this;
		}

		template<std::convertible_to<double> T>
		constexpr approx& scale(T const& scale) {
			_scale = static_cast<double>(scale);
			return *this;
		}

		constexpr approx operator-() {
			approx result = *this;
			result._value = -_value;
			return result;
		}

		template<std::convertible_to<double> T>
		constexpr bool operator==(T rhs) const {
			return operator==(static_cast<double>(rhs));
		}

		constexpr bool operator==(double rhs) const {
			// Thanks to Richard Harris for his help refining this formula
			return OpenVic::utility::abs(rhs - _value) < _epsilon * (_scale + std::max<double>(OpenVic::utility::abs(rhs), OpenVic::utility::abs(_value)));
		}

		template<std::convertible_to<double> T>
		constexpr std::partial_ordering operator<=>(T rhs) const {
			return operator<=>(static_cast<double>(rhs));
		}

		constexpr std::partial_ordering operator<=>(double rhs) const {
			return _value <=> rhs;
		}

		double _epsilon;
		double _scale;
		double _value;
	};

	namespace approx_literal {
		constexpr approx operator""_a(long double value) {
			return value;
		}
	};

	using approx_vec2 = OpenVic::vec2_t<approx>;
	using approx_vec3 = OpenVic::vec3_t<approx>;
	using approx_vec4 = OpenVic::vec4_t<approx>;
}

namespace snitch {
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::testing::approx const& s) {
		return append(ss, s._value);
	}
}
