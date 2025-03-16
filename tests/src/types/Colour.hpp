#pragma once

#include <cmath>

#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace snitch {
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	[[nodiscard]] inline static constexpr bool append( //
		snitch::small_string_span ss, OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits> const& s
	) noexcept {
		if constexpr (std::decay_t<decltype(s)>::colour_traits::has_alpha) {
			return append(ss, "(", (int64_t)s.red, ",", (int64_t)s.green, ",", (int64_t)s.blue, ",", (int64_t)s.alpha, ")");
		} else {
			return append(ss, "(", (int64_t)s.red, ",", (uint64_t)s.green, ",", (int64_t)s.blue, ")");
		}
	}
}
