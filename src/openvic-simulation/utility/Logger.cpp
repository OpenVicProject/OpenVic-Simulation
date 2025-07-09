#include "Logger.hpp"

using namespace OpenVic;

thread_local memory::vector<std::string_view> Logger::log_scope_stack {};
thread_local size_t Logger::log_scope_index_plus_one = 0;
