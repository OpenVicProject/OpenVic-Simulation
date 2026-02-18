#pragma once

#include <cstdint>

namespace OpenVic {
    enum struct OptionalBool : std::uint8_t {
        UNSPECIFIED = 0,
        TRUE = 1,
        FALSE = 2
    };
}