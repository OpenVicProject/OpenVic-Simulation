#pragma once

#include <string_view>

#include "openvic-simulation/core/memory/StringMap.hpp"
#include "openvic-simulation/core/memory/StringSet.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic::memory {

	struct FlagStrings {
	private:
		memory::string_set_t PROPERTY(flags);
		memory::string name;

	public:
		FlagStrings(std::string_view new_name);
		FlagStrings(FlagStrings&&) = default;

		bool set_flag(std::string_view flag, bool warn);
		bool clear_flag(std::string_view flag, bool warn);
		bool has_flag(std::string_view flag) const;

		// Go through the map of flags setting or clearing each based on whether
		// its value is true or false (used for applying history entries).
		bool apply_flag_map(memory::string_map_t<bool> const& flag_map, bool warn);
	};
}
