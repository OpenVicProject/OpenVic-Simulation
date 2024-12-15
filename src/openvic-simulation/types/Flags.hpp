#pragma once

#include <string_view>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

    struct Flags {
    private:
        string_set_t PROPERTY(flags);

    public:
        bool set_flag(std::string_view flag, bool warn);
        bool clear_flag(std::string_view flag, bool warn);
        bool has_flag(std::string_view flag) const;
    };
}