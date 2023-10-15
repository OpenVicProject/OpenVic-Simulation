#include "Logger.hpp"

#include <iostream>

using namespace OpenVic;

void Logger::set_logger_funcs() {
	Logger::set_info_func([](std::string&& str) { std::cout << str; });
	Logger::set_warning_func([](std::string&& str) { std::cerr << str; });
	Logger::set_error_func([](std::string&& str) { std::cerr << str; });
}

char const* Logger::get_filename(char const* filepath, char const* default_path) {
	if (filepath == nullptr) return default_path;
	char const* last_slash = filepath;
	while (*filepath != '\0') {
		if (*filepath == '\\' || *filepath == '/') last_slash = filepath + 1;
		filepath++;
	}
	if (*last_slash == '\0') return default_path;
	return last_slash;
}
