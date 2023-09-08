#pragma once

#include <functional>
#include <queue>
#include <sstream>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif

namespace OpenVic {

#ifndef __cpp_lib_source_location
#include <string>
	// Implementation of std::source_location for compilers that do not support it
	// Note: uses non-standard extensions that are supported by Clang, GCC, and MSVC
	// https://clang.llvm.org/docs/LanguageExtensions.html#source-location-builtins
	// https://stackoverflow.com/a/67970107
	class source_location {
		std::string _file;
		int _line;
		std::string _function;

	public:
		source_location(std::string f, int l, std::string n) : _file(f), _line(l), _function(n) {}
		static source_location current(std::string f = __builtin_FILE(), int l = __builtin_LINE(), std::string n = __builtin_FUNCTION()) {
			return source_location(f, l, n);
		}

		inline char const* file_name() const { return _file.c_str(); }
		inline int line() const { return _line; }
		inline char const* function_name() const { return _function.c_str(); }
	};
#endif

	class Logger {
		using log_func_t = std::function<void(std::string&&)>;
		using log_queue_t = std::queue<std::string>;

#ifdef __cpp_lib_source_location
		using source_location = std::source_location;
#else
		using source_location = OpenVic::source_location;
#endif

		static char const* get_filename(char const* filepath);

		template<typename... Ts>
		struct log {
			log(log_func_t log_func, log_queue_t& log_queue, Ts&&... ts, source_location const& location) {
				std::stringstream stream;
				stream << "\n" << get_filename(location.file_name()) << "("
						<< location.line() << ") `" << location.function_name() << "`: ";
				((stream << std::forward<Ts>(ts)), ...);
				stream << std::endl;
				log_queue.push(stream.str());
				if (log_func) {
					do {
						log_func(std::move(log_queue.front()));
						log_queue.pop();
					} while (!log_queue.empty());
				}
			}
		};

#define LOG_FUNC(name)																			\
	private:																					\
		static log_func_t name##_func;															\
		static log_queue_t name##_queue;														\
	public:																						\
		static void set_##name##_func(log_func_t log_func) {									\
			name##_func = log_func;																\
		}																						\
		template <typename... Ts>																\
		struct name {																			\
			name(Ts&&... ts, source_location const& location = source_location::current()) {	\
				log<Ts...>{ name##_func, name##_queue, std::forward<Ts>(ts)..., location };		\
			}																					\
		};																						\
		template <typename... Ts>																\
		name(Ts&&...) -> name<Ts...>;

		LOG_FUNC(info)
		LOG_FUNC(error)

#undef LOG_FUNC
	};
}
