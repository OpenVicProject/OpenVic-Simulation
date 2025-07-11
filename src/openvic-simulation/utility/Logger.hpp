#pragma once

#include <cstddef>
#include <iostream>
#include <mutex>
#include <string_view>
#include <vector>
#include <version>

#include <function2/function2.hpp>

#ifdef __cpp_lib_source_location
#include <source_location>
#endif

#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

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

		source_location(std::string f, int l, std::string n) : _file(f), _line(l), _function(n) {}

	public:
		static source_location current(
			std::string f = __builtin_FILE(), int l = __builtin_LINE(), std::string n = __builtin_FUNCTION()
		) {
			return source_location(f, l, n);
		}

		inline char const* file_name() const {
			return _file.c_str();
		}
		inline int line() const {
			return _line;
		}
		inline char const* function_name() const {
			return _function.c_str();
		}
	};
#endif

	class Logger final {
		using log_func_t = fu2::function_view<void(memory::string&&)>;
		using log_queue_t = memory::queue<memory::string>;

	public:
#ifdef __cpp_lib_source_location
		using source_location = std::source_location;
#else
		using source_location = OpenVic::source_location;
#endif

	private:
		struct log_channel_t {
			log_func_t func;
			log_queue_t queue;
			size_t message_count;
		};

		static inline std::mutex log_mutex;
		static thread_local memory::vector<std::string_view> log_scope_stack;
		static thread_local size_t log_scope_index_plus_one;

		template<typename... Args>
		struct log {
			log(log_channel_t& log_channel, Args&&... args, source_location const& location) {
				if (log_scope_index_plus_one < log_scope_stack.size()) {
					const std::string_view log_scope = log_scope_stack[log_scope_index_plus_one];
					log_scope_index_plus_one++;
					//recursive
					info(std::string(log_scope_index_plus_one, '>'), " Enter scope: ", log_scope);
				}

				const std::lock_guard<std::mutex> lock { log_mutex };

				memory::stringstream stream;
				stream << StringUtils::get_filename(location.file_name()) << "("
					/* Function name removed to reduce clutter. It is already included
					* in Godot's print functions, so this was repeating it. */
					//<< location.line() << ") `" << location.function_name() << "`: ";
					<< location.line() << "): ";
				((stream << std::forward<Args>(args)), ...);
				stream << std::endl;
				log_channel.queue.push(stream.str());
				if (log_channel.func) {
					do {
						log_channel.func(std::move(log_channel.queue.front()));
						log_channel.queue.pop();
						/* Only count printed messages, so that message_count matches what is seen in the console. */
						log_channel.message_count++;
					} while (!log_channel.queue.empty());
				}
			}
		};

	public:
		static void enter_log_scope(const std::string_view log_scope) {
			log_scope_stack.push_back(log_scope);
		}
		static void exit_log_scope(const std::string_view log_scope) {
			const std::string_view to_pop = log_scope_stack.back();
			if (to_pop != log_scope) {
				error("Tried exiting log scope \"", log_scope, "\" but \"", to_pop, "\" would be removed instead. No log scope was removed instead.");
				return;
			}
			log_scope_stack.pop_back();

			const size_t max_log_scope_index_plus_one = log_scope_stack.size();
			if (max_log_scope_index_plus_one < log_scope_index_plus_one) {
				const std::string prefix = std::string(log_scope_index_plus_one, '<');
				log_scope_index_plus_one = max_log_scope_index_plus_one;
				info(prefix, " Exit scope: ", log_scope);
			}
		}

		static void set_logger_funcs() {
			set_info_func([](memory::string&& str) {
				std::cout << "[INFO] " << str;
			});
			set_warning_func([](memory::string&& str) {
				std::cerr << "[WARNING] " << str;
			});
			set_error_func([](memory::string&& str) {
				std::cerr << "[ERROR] " << str;
			});
		}

#define LOG_FUNC(name) \
private: \
	static inline log_channel_t name##_channel {}; \
\
public: \
	static inline void set_##name##_func(log_func_t log_func) { \
		name##_channel.func = log_func; \
	} \
	static inline size_t get_##name##_count() { \
		return name##_channel.message_count; \
	} \
	template<typename... Args> \
	struct name { \
		name(Args&&... args, source_location const& location = source_location::current()) { \
			log<Args...> { name##_channel, std::forward<Args>(args)..., location }; \
		} \
	}; \
	template<typename... Args> \
	name(Args&&...) -> name<Args...>;

		LOG_FUNC(info)
		LOG_FUNC(warning)
		LOG_FUNC(error)

#undef LOG_FUNC

		template<typename... Args>
		static inline constexpr void warn_or_error(bool warn, Args&&... args) {
			if (warn) {
				warning(std::forward<Args>(args)...);
			} else {
				error(std::forward<Args>(args)...);
			}
		}
	};
}
