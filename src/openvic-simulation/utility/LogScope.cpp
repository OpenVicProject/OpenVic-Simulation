#include "LogScope.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

LogScope::LogScope(std::string const& new_text)
	: text { new_text }
{
	Logger::enter_log_scope(text);
}

LogScope::LogScope(std::string&& new_text)
	: text { std::move(new_text) }
{
	Logger::enter_log_scope(text);
}

LogScope::~LogScope() {
	Logger::exit_log_scope(text);
}