#include "Flags.hpp"

#include "openvic-simulation/utility/Logger.hpp"


using namespace OpenVic;

bool Flags::set_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to set empty  flag for  ");
		return false;
	}
	if (!flags.emplace(flag).second && warn) {
		Logger::warning(
			"Attempted to set  flag \"", flag, "\" for  ", ": already set!"
		);
	}
	return true;
}

bool Flags::clear_flag(std::string_view flag, bool warn) {
	if (flag.empty()) {
		Logger::error("Attempted to clear empty  flag from ");
		return false;
	}
	if (flags.erase(flag) == 0 && warn) {
		Logger::warning(
			"Attempted to clear  flag \"", flag, "\" from  ", ": not set!"
		);
	}
	return true;
}

bool Flags::has_flag(std::string_view flag) const {
	return flags.contains(flag);
}