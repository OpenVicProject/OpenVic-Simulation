#pragma once

#include <cstddef>
#include <memory>
#include <source_location>
#include <string_view>

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include "openvic-simulation/utility/Containers.hpp"

namespace spdlog {
	template<typename... Args>
	struct log_s {
		log_s(
			level::level_enum level, format_string_t<Args...> fmt, Args&&... args,
			std::source_location const& location = std::source_location::current()
		) {
			memory_buf_t buf;
			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));

			default_logger_raw()->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level,
				string_view_t(buf.data(), buf.size())
			);
		}

		log_s(
			std::shared_ptr<logger> const& logger, level::level_enum level, format_string_t<Args...> fmt, Args&&... args,
			std::source_location const& location = std::source_location::current()
		) {
			memory_buf_t buf;
			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...));

			logger->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level,
				string_view_t(buf.data(), buf.size())
			);
		}
	};

	template<typename... Args>
	log_s(level::level_enum level, format_string_t<Args...> fmt, Args&&... args) -> log_s<Args...>;
	template<typename... Args>
	log_s( //
		std::shared_ptr<logger> const& logger, level::level_enum level, format_string_t<Args...> fmt, Args&&... args
	) -> log_s<Args...>;

	template<typename T>
	struct log_s<T> {
		log_s(
			level::level_enum level, format_string_t<T> fmt, T&& arg,
			std::source_location const& location = std::source_location::current()
		) {
			default_logger_raw()->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level, fmt,
				std::forward<T>(arg)
			);
		}
		log_s(level::level_enum level, T&& arg, std::source_location const& location = std::source_location::current()) {
			default_logger_raw()->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level,
				std::forward<T>(arg)
			);
		}

		log_s(
			std::shared_ptr<logger> const& logger, level::level_enum level, format_string_t<T> fmt, T&& arg,
			std::source_location const& location = std::source_location::current()
		) {
			logger->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level, fmt,
				std::forward<T>(arg)
			);
		}

		log_s(
			std::shared_ptr<logger> const& logger, level::level_enum level, T&& arg,
			std::source_location const& location = std::source_location::current()
		) {
			logger->log(
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, level,
				std::forward<T>(arg)
			);
		}
	};

	template<typename T>
	log_s(level::level_enum lvl, T&& msg) -> log_s<T>;
	template<typename T>
	log_s(std::shared_ptr<logger> const& logger, level::level_enum lvl, T&& msg) -> log_s<T>;

#define DEFINE_LOG_LEVEL(NAME, LEVEL) \
	template<typename... Args> \
	struct NAME { \
		NAME( \
			format_string_t<Args...> fmt, Args&&... args, \
			std::source_location const& location = std::source_location::current() \
		) { \
			memory_buf_t buf; \
			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...)); \
\
			default_logger_raw()->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, \
				string_view_t(buf.data(), buf.size()) \
			); \
		} \
\
		NAME( \
			std::shared_ptr<logger> const& logger, format_string_t<Args...> fmt, Args&&... args, \
			std::source_location const& location = std::source_location::current() \
		) { \
			memory_buf_t buf; \
			fmt::vformat_to(fmt::appender(buf), fmt, fmt::make_format_args(args...)); \
\
			logger->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, \
				string_view_t(buf.data(), buf.size()) \
			); \
		} \
	}; \
\
	template<typename... Args> \
	NAME(format_string_t<Args...> fmt, Args&&... args) -> NAME<Args...>; \
	template<typename... Args> \
	NAME(std::shared_ptr<logger> const& logger, format_string_t<Args...> fmt, Args&&... args) -> NAME<Args...>; \
\
	template<typename T> \
	struct NAME<T> { \
		NAME(format_string_t<T> fmt, T&& arg, std::source_location const& location = std::source_location::current()) { \
			default_logger_raw()->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, fmt, \
				std::forward<T>(arg) \
			); \
		} \
		NAME(T&& arg, std::source_location const& location = std::source_location::current()) { \
			default_logger_raw()->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, \
				std::forward<T>(arg) \
			); \
		} \
\
		NAME( \
			std::shared_ptr<logger> const& logger, format_string_t<T> fmt, T&& arg, \
			std::source_location const& location = std::source_location::current() \
		) { \
			logger->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, fmt, \
				std::forward<T>(arg) \
			); \
		} \
\
		NAME( \
			std::shared_ptr<logger> const& logger, T&& arg, \
			std::source_location const& location = std::source_location::current() \
		) { \
			logger->log( \
				source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() }, LEVEL, \
				std::forward<T>(arg) \
			); \
		} \
	}; \
\
	template<typename T> \
	NAME(T&& msg) -> NAME<T>; \
	template<typename T> \
	NAME(std::shared_ptr<logger> const& logger, T&& msg) -> NAME<T>;

	DEFINE_LOG_LEVEL(warn_s, ::spdlog::level::warn);
	DEFINE_LOG_LEVEL(error_s, ::spdlog::level::err);
	DEFINE_LOG_LEVEL(critical_s, ::spdlog::level::critical);

#undef DEFINE_LOG_LEVEL

	struct scope {
		using string = OpenVic::memory::string;

		scope(scope const&) = delete;
		scope& operator=(scope const&) = delete;
		scope(scope&&) = delete;
		scope& operator=(scope&&) = delete;

		scope(std::string_view text) : _logger { spdlog::default_logger_raw()->clone(std::string { text }) } {
			if (stack.empty()) {
				stack.emplace(spdlog::default_logger());
			}
			spdlog::set_default_logger(_logger);
			stack.emplace(_logger);
		}

		~scope() {
			std::shared_ptr<spdlog::logger> back = stack.top();
			if (back->name() != _logger->name()) {
				error(
					"Tried exiting scope \"{}\" but \"{}\" would be removed instead. No scope was removed.", _logger->name(),
					back->name()
				);
				return;
			}
			stack.pop();
			spdlog::set_default_logger(stack.top());

			size_t scope_size = stack.size();
			if (scope_size < stack_index) {
				stack_index = scope_size;
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
				info("{}< Exit scope", stack_index);
#endif
			}
		}

		std::shared_ptr<spdlog::logger>& logger() {
			return _logger;
		}
		std::shared_ptr<spdlog::logger> const& logger() const {
			return _logger;
		}

	private:
		std::shared_ptr<spdlog::logger> _logger;

		inline static thread_local OpenVic::memory::stack<std::shared_ptr<spdlog::logger>> stack;
		inline static thread_local size_t stack_index;
	};
}
