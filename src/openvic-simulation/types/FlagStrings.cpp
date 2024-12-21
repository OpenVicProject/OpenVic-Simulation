#include "FlagStrings.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

FlagStrings::FlagStrings(std::string_view new_name) : name { new_name } {}

bool FlagStrings::set_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to set empty ", name, " flag!");
		return false;
	}

	if (!flags.emplace(flag).second && warn) {
		Logger::warning("Attempted to set ", name, " flag \"", flag, "\": already set!");
	}

	return true;
}

bool FlagStrings::clear_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to clear empty ", name, " flag!");
		return false;
	}

	if (flags.erase(flag) == 0 && warn) {
		Logger::warning("Attempted to clear ", name, " flag \"", flag, "\": not set!");
	}

	return true;
}

bool FlagStrings::has_flag(std::string_view flag) const {
	return flags.contains(flag);
}
