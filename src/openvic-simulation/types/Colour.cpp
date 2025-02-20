#include "openvic-simulation/types/Colour.hpp" // IWYU pragma: keep

namespace OpenVic {
	template struct basic_colour_t<std::uint8_t, std::uint32_t>;
	template struct basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;
}
