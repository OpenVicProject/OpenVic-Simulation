#pragma once

#include <algorithm>
#include <cstdint>

namespace OpenVic {
	// Represents a 24-bit RGB integer OR a 32-bit ARGB integer
	using colour_t = uint32_t;
	/* When colour_t is used as an identifier, NULL_COLOUR is disallowed
	 * and should be reserved as an error value.
	 * When colour_t is used in a purely graphical context, NULL_COLOUR
	 * should be allowed.
	 */
	static constexpr colour_t NULL_COLOUR = 0, FULL_COLOUR = 0xFF, MAX_COLOUR_RGB = 0xFFFFFF;
	constexpr colour_t float_to_colour_byte(float f, float min = 0.0f, float max = 1.0f) {
		return static_cast<colour_t>(std::clamp(min + f * (max - min), min, max) * 255.0f);
	}
	constexpr colour_t fraction_to_colour_byte(int n, int d, float min = 0.0f, float max = 1.0f) {
		return float_to_colour_byte(static_cast<float>(n) / static_cast<float>(d), min, max);
	}
	constexpr colour_t float_to_alpha_value(float a) {
		return float_to_colour_byte(a) << 24;
	}
	constexpr float colour_byte_to_float(colour_t colour) {
		return std::clamp(static_cast<float>(colour) / 255.0f, 0.0f, 1.0f);
	}
}
