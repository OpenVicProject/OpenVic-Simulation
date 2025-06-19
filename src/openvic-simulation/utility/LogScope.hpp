#pragma once

#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct LogScope {
	private:
		const std::string PROPERTY(text);
	public:
		explicit LogScope(std::string const& new_text);
		LogScope(std::string&& new_text);
		~LogScope();
	};
}