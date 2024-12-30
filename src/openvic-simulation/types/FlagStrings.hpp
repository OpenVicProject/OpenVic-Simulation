#pragma once

#include <string>
#include <string_view>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	struct FlagStrings {
	private:
		string_set_t PROPERTY(flags);
		std::string name;

	public:
		FlagStrings(std::string_view new_name);
		FlagStrings(FlagStrings&&) = default;

		bool set_flag(std::string_view flag, bool warn);
		bool clear_flag(std::string_view flag, bool warn);
		bool has_flag(std::string_view flag) const;
	};
}