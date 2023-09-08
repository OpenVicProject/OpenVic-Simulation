#include "Logger.hpp"

#include <iostream>

using namespace OpenVic;

Logger::log_func_t Logger::info_func {};
Logger::log_queue_t Logger::info_queue {};
Logger::log_func_t Logger::error_func {};
Logger::log_queue_t Logger::error_queue {};

char const* Logger::get_filename(char const* filepath) {
	if (filepath == nullptr) return nullptr;
	char const* last_slash = filepath;
	while (*filepath != '\0') {
		if (*filepath == '\\' || *filepath == '/') last_slash = filepath + 1;
		filepath++;
	}
	return last_slash;
}
