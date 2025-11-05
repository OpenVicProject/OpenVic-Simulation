#pragma once

#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/core/object/Vector.hpp"

#include "Numeric.hpp"
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace OpenVic::testing {
	static constexpr double SQRT2 = 1.4142135623730950488016887242;
	static constexpr double SQRT13 = 0.57735026918962576450914878050196;
	static constexpr double SQRT3 = 1.7320508075688772935274463415059;

	inline constexpr fvec2_t parse_unsafe_v2(double x, double y) {
		return fvec2_t(fixed_point_t::parse_unsafe(x), fixed_point_t::parse_unsafe(y));
	}

	inline constexpr fvec3_t parse_unsafe_v3(double x, double y, double z) {
		return fvec3_t(fixed_point_t::parse_unsafe(x), fixed_point_t::parse_unsafe(y), fixed_point_t::parse_unsafe(z));
	}

	inline constexpr fvec4_t parse_unsafe_v4(double x, double y, double z, double w) {
		return fvec4_t(
			fixed_point_t::parse_unsafe(x), fixed_point_t::parse_unsafe(y), fixed_point_t::parse_unsafe(z),
			fixed_point_t::parse_unsafe(w)
		);
	}

	inline dvec2_t approx_value(fvec2_t value, dvec2_t compare) {
		return { approx_value(value.x, compare.x), approx_value(value.y, compare.y) };
	}

	inline dvec3_t approx_value(fvec3_t value, dvec3_t compare) {
		return { approx_value(value.x, compare.x), approx_value(value.y, compare.y), approx_value(value.z, compare.z) };
	}

	inline dvec4_t approx_value(fvec4_t value, dvec4_t compare) {
		return { //
				 approx_value(value.x, compare.x), //
				 approx_value(value.y, compare.y), //
				 approx_value(value.z, compare.z), //
				 approx_value(value.w, compare.w)
		};
	}

	inline dvec2_t approx_value(dvec2_t value, dvec2_t compare) {
		return { approx_value(value.x, compare.x), approx_value(value.y, compare.y) };
	}

	inline dvec3_t approx_value(dvec3_t value, dvec3_t compare) {
		return { approx_value(value.x, compare.x), approx_value(value.y, compare.y), approx_value(value.z, compare.z) };
	}

	inline dvec4_t approx_value(dvec4_t value, dvec4_t compare) {
		return { //
				 approx_value(value.x, compare.x), //
				 approx_value(value.y, compare.y), //
				 approx_value(value.z, compare.z), //
				 approx_value(value.w, compare.w)
		};
	}
}

namespace snitch {
	template<typename T>
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::vec2_t<T> const& s) {
		return append(ss, "(", s.x, ",", s.y, ")");
	}

	template<typename T>
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::vec3_t<T> const& s) {
		return append(ss, "(", s.x, ",", s.y, ",", s.z, ")");
	}

	template<typename T>
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::vec4_t<T> const& s) {
		return append(ss, "(", s.x, ",", s.y, ",", s.z, ",", s.w, ")");
	}
}
