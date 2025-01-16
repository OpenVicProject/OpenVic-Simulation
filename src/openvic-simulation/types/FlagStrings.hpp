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

		// Go through the map of flags setting or clearing each based on whether
		// its value is true or false (used for applying history entries).
		bool apply_flag_map(string_map_t<bool> const& flag_map, bool warn);
	};
}
