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

	class Logger final {
		using log_func_t = std::function<void(std::string&&)>;
		using log_queue_t = std::queue<std::string>;

#ifdef __cpp_lib_source_location
		using source_location = std::source_location;
#else
		using source_location = OpenVic::source_location;
#endif

	public:
		static void set_logger_funcs();
		static char const* get_filename(char const* filepath, char const* default_path = nullptr);

	private:
		struct log_channel_t {
			log_func_t func;
			log_queue_t queue;
		};

		template<typename... Ts>
		struct log {
			log(log_channel_t& log_channel, Ts&&... ts, source_location const& location) {
				std::stringstream stream;
				stream << "\n" << get_filename(location.file_name()) << "("
						//<< location.line() << ") `" << location.function_name() << "`: ";
						<< location.line() << "): ";
				((stream << std::forward<Ts>(ts)), ...);
				stream << std::endl;
				log_channel.queue.push(stream.str());
				if (log_channel.func) {
					do {
						log_channel.func(std::move(log_channel.queue.front()));
						log_channel.queue.pop();
					} while (!log_channel.queue.empty());
				}
			}
		};

#define LOG_FUNC(name) \
	private: \
		static inline log_channel_t name##_channel{}; \
	public: \
		static inline void set_##name##_func(log_func_t log_func) { \
			name##_channel.func = log_func; \
		} \
		template<typename... Ts> \
		struct name { \
			name(Ts&&... ts, source_location const& location = source_location::current()) { \
				log<Ts...>{ name##_channel, std::forward<Ts>(ts)..., location }; \
			} \
		}; \
		template<typename... Ts> \
		name(Ts&&...) -> name<Ts...>;

		LOG_FUNC(info)
		LOG_FUNC(warning)
		LOG_FUNC(error)

#undef LOG_FUNC
	};
}
