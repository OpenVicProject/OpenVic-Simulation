#pragma once

#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/base.h>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "openvic-simulation/core/template/Concepts.hpp"

#ifdef _WIN32
#include <spdlog/sinks/wincolor_sink.h>
#else
#include <spdlog/sinks/ansicolor_sink.h>
#endif

namespace OpenVic::dataloader {
	static constexpr std::string_view logger_name = "dataloader logger";

	class logger_registry {
	public:
		logger_registry(logger_registry const&) = delete;
		logger_registry& operator=(logger_registry const&) = delete;

		std::shared_ptr<spdlog::logger> default_logger();

		// Return raw ptr to the default logger.
		// To be used directly by the dataloader logger default api
		// This make the default API faster, but cannot be used concurrently with set_default_logger().
		// e.g do not call set_default_logger() from one thread while calling spdlog::info() from another.
		spdlog::logger* get_default_raw();

		static logger_registry& instance();

	private:
		logger_registry();
		~logger_registry();

		std::mutex logger_map_mutex;
		std::shared_ptr<spdlog::logger> logger;
	};

	std::shared_ptr<spdlog::logger> default_logger();
	spdlog::logger* default_logger_raw();

	namespace log {
		template<typename... Args>
		struct fstring : spdlog::format_string_t<Args...> {
			using t = fstring;
			using base_type = spdlog::format_string_t<Args...>;

			std::source_location loc;

			template<size_t N>
			consteval FMT_ALWAYS_INLINE fstring(
			    const char (&s)[N], std::source_location const& loc = std::source_location::current()
			) : base_type { s }, loc { loc } {}

			template<typename S, FMT_ENABLE_IF(std::is_convertible<S const&, fmt::string_view>::value)>
			consteval FMT_ALWAYS_INLINE fstring(S const& s, std::source_location const& loc = std::source_location::current()) :
			    base_type { s }, loc { loc } {}

			template<
			    typename S,
			    FMT_ENABLE_IF(
			        (std::is_base_of<fmt::detail::compile_string, S>::value) && std::is_same<typename S::char_type, char>::value
			    )>
			fstring(S const&, std::source_location const& loc = std::source_location::current()) :
			    base_type { S() }, loc { loc } {}

			fstring(fmt::runtime_format_string<> fmt, std::source_location const& loc = std::source_location::current()) :
			    base_type(fmt), loc { loc } {}
		};

		template<typename... T>
		using format_string = typename fstring<T...>::t;

		namespace detail {
			template<typename T>
			concept not_convertible_to_any_format_string = !spdlog::is_convertible_to_any_format_string<T const&>::value;

			template<typename T>
			concept runtime_log_string =
			    !specialization_of<T, fstring> && spdlog::is_convertible_to_any_format_string<T const&>::value &&
			    !constexpr_constructible<T>;

			template<typename T>
			concept runtime_string =
			    spdlog::is_convertible_to_any_format_string<T const&>::value && !constexpr_constructible<T>;
		}

		inline void vlog(
		    std::source_location const& loc, spdlog::level::level_enum lvl, spdlog::string_view_t fmt, fmt::format_args args
		) {
			spdlog::memory_buf_t buf;
			fmt::vformat_to(fmt::appender(buf), fmt, args);

			default_logger_raw()->log(
			    spdlog::source_loc { loc.file_name(), static_cast<int>(loc.line()), loc.function_name() },
			    lvl,
			    spdlog::string_view_t { buf.data(), buf.size() }
			);
		}

		template<typename... Args>
		inline void log(spdlog::level::level_enum lvl, format_string<Args...> fmt, Args&&... args) {
			vlog(fmt.loc, lvl, fmt, fmt::make_format_args(args...));
		}

		template<typename... Args>
		inline void trace(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::trace, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline void debug(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline void info(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::info, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline void warn(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline void error(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::err, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		inline void critical(format_string<Args...> fmt, Args&&... args) {
			log(spdlog::level::critical, fmt, std::forward<Args>(args)...);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void log(
		    spdlog::level::level_enum lvl, T const& v, std::source_location const& loc = std::source_location::current()
		) {
			vlog(loc, lvl, "{}", fmt::make_format_args(v));
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void trace(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::trace, v, loc);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void debug(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::debug, v, loc);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void info(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::info, v, loc);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void warn(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::warn, v, loc);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void error(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::err, v, loc);
		}

		template<detail::not_convertible_to_any_format_string T>
		inline void critical(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::critical, v, loc);
		}

		template<detail::runtime_log_string T>
		inline void log(
		    spdlog::level::level_enum lvl, T const& v, std::source_location const& loc = std::source_location::current()
		) {
			vlog(loc, lvl, "{}", fmt::make_format_args(v));
		}

		template<detail::runtime_string T>
		inline void trace(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::trace, v, loc);
		}

		template<detail::runtime_string T>
		inline void debug(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::debug, v, loc);
		}

		template<detail::runtime_string T>
		inline void info(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::info, v, loc);
		}

		template<detail::runtime_string T>
		inline void warn(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::warn, v, loc);
		}

		template<detail::runtime_string T>
		inline void error(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::err, v, loc);
		}

		template<detail::runtime_string T>
		inline void critical(T const& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::critical, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void log(
		    spdlog::level::level_enum lvl, T&& v, std::source_location const& loc = std::source_location::current()
		) {
			vlog(loc, lvl, "{}", fmt::make_format_args(v));
		}

		template<std::same_as<std::string> T>
		inline void trace(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::trace, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void debug(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::debug, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void info(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::info, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void warn(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::warn, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void error(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::err, v, loc);
		}

		template<std::same_as<std::string> T>
		inline void critical(T&& v, std::source_location const& loc = std::source_location::current()) {
			log(spdlog::level::critical, v, loc);
		}
	}
}
