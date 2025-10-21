#include "FlagStrings.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

FlagStrings::FlagStrings(std::string_view new_name) : name { new_name } {}

bool FlagStrings::set_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		spdlog::error_s("Attempted to set empty {} flag!", name);
		return false;
	}

	if (!flags.emplace(flag).second && warn) {
		spdlog::warn_s(
			"Attempted to set {} flag \"{}\": already set!",
			name, flag
		);
	}

	return true;
}

bool FlagStrings::clear_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		spdlog::error_s("Attempted to clear empty {} flag!", name);
		return false;
	}

	if (flags.erase(flag) == 0 && warn) {
		spdlog::warn_s(
			"Attempted to clear {} flag \"{}\": not set!",
			name, flag
		);
	}

	return true;
}

bool FlagStrings::has_flag(std::string_view flag) const {
	return flags.contains(flag);
}

bool FlagStrings::apply_flag_map(string_map_t<bool> const& flag_map, bool warn) {
	bool ret = true;

	for (auto const& [flag, set] : flag_map) {
		if (set) {
			ret &= set_flag(flag, warn);
		} else {
			ret &= clear_flag(flag, warn);
		}
	}

	return ret;
}
