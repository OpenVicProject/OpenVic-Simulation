#pragma once

#include <concepts> // IWYU pragma: keep
#include <type_traits> // IWYU pragma: keep

#include <spdlog/spdlog.h> // IWYU pragma: keep

#include "openvic-simulation/core/error/Error.hpp" // IWYU pragma: keep for error
#include "openvic-simulation/core/Typedefs.hpp" // IWYU pragma: keep for macros

// Based heavily on https://github.com/godotengine/godot/blob/34d06658a85845111a50db9e485ec4a0701d4298/core/error/error_macros.h

#ifndef OV_GENERATE_TRAP
#if defined(_MSC_VER) && !defined(__clang__)
/**
 * Don't use OV_GENERATE_TRAP() directly, should only be used be the macros below.
 */
#define OV_GENERATE_TRAP() __debugbreak()
#else
/**
 * Don't use OV_GENERATE_TRAP() directly, should only be used be the macros below.
 */
#define OV_GENERATE_TRAP() __builtin_trap()
#endif
#endif // OV_GENERATE_TRAP

/**
 * Error macros.
 * WARNING: These macros work in the opposite way to assert().
 *
 * Unlike exceptions and asserts, these macros try to maintain consistency and stability.
 * In most cases, bugs and/or invalid data are not fatal. They should never allow a perfectly
 * running application to fail or crash.
 * Always try to return processable data, so the engine can keep running well.
 * Use the _MSG versions to print a meaningful message to help with debugging.
 *
 * The `((void)0)` no-op statement is used as a trick to force us to put a semicolon after
 * those macros, making them look like proper statements.
 * The if wrappers are used to ensure that the macro replacement does not trigger unexpected
 * issues when expanded e.g. after an `if (cond) OV_ERR_FAIL();` without braces.
 */

/**
 * Evaluate the expression. If it results in an Error != OK, silently return the error.
 */
#define OV_RETURN_IF_ERROR(m_exp) \
	if (::OpenVic::Error _err_propagate_error = (m_exp); OV_unlikely(_err_propagate_error != ::OpenVic::Error::OK)) { \
		static_assert( \
			std::same_as<std::decay_t<decltype(m_exp)>, ::OpenVic::Error>, \
			"OV_RETURN_IF_ERROR expects an Error-returning expression" \
		); \
		return _err_propagate_error; \
	} else \
		((void)0)

// Index out of bounds error macros.
// These macros should be used instead of `OV_ERR_FAIL_COND` for bounds checking.

// Integer index out of bounds error macros.

/**
 * Try using `OV_ERR_FAIL_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns.
 */
#define OV_ERR_FAIL_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_ERROR("Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns.
 */
#define OV_ERR_FAIL_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_INDEX_V(m_index, m_size, m_retval) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"Index {} = {} is out of bounds ({} = {}). Returning: {}", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size), \
			_OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_INDEX_V_MSG(m_index, m_size, m_retval, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}). Returning: {}", (m_msg), _OV_STR(m_index), (m_index), \
			_OV_STR(m_size), (m_size), _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_INDEX_MSG` or `OV_ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the application crashes.
 */
#define OV_CRASH_BAD_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_CRITICAL("Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size)); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_INDEX_MSG` or `OV_ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the application crashes.
 */
#define OV_CRASH_BAD_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		SPDLOG_CRITICAL( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

// Unsigned integer index out of bounds error macros.

/**
 * Try using `OV_ERR_FAIL_UNSIGNED_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns.
 */
#define OV_ERR_FAIL_UNSIGNED_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_ERROR("Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns.
 */
#define OV_ERR_FAIL_UNSIGNED_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_UNSIGNED_INDEX_V(m_index, m_size, m_retval) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"Index {} = {} is out of bounds ({} = {}). Returning: {}", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size), \
			_OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_UNSIGNED_INDEX_V_MSG(m_index, m_size, m_retval, m_msg) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_ERROR( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}). Returning: {}", (m_msg), _OV_STR(m_index), (m_index), \
			_OV_STR(m_size), (m_size), _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_UNSIGNED_INDEX_MSG` or `OV_ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the application crashes.
 */
#define OV_CRASH_BAD_UNSIGNED_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_CRITICAL("Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size)); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_UNSIGNED_INDEX_MSG` or `OV_ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the application crashes.
 */
#define OV_CRASH_BAD_UNSIGNED_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		SPDLOG_CRITICAL( \
			"{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

// Null reference error macros.

/**
 * Try using `OV_ERR_FAIL_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns.
 */
#define OV_ERR_FAIL_NULL(m_param) \
	if (OV_unlikely(m_param == nullptr)) { \
		SPDLOG_ERROR("Parameter \"{}\" is null.", _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 */
#define OV_ERR_FAIL_NULL_MSG(m_param, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		SPDLOG_ERROR("{}\n\tParameter \"{}\" is null.", (m_msg), _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_NULL_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_NULL_V(m_param, m_retval) \
	if (OV_unlikely(m_param == nullptr)) { \
		SPDLOG_ERROR("Parameter \"{}\" is null. Returning: {}", _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_NULL_V_MSG(m_param, m_retval, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		SPDLOG_ERROR("{}\n\tParameter \"{}\" is null. Returning: {}", (m_msg), _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use OV_ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use OV_ERR_FAIL_INDEX_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns.
 */
#define OV_ERR_FAIL_COND(m_cond) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("Condition \"{}\" is true.", _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns.
 *
 * If checking for null use OV_ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use OV_ERR_FAIL_INDEX_MSG instead.
 */
#define OV_ERR_FAIL_COND_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("{}\n\tCondition \"{}\" is true.", (m_msg), _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use OV_ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use OV_ERR_FAIL_INDEX_V_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_COND_V(m_cond, m_retval) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("Condition \"{}\" is true. Returning: {}", _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns `m_retval`.
 *
 * If checking for null use OV_ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use OV_ERR_FAIL_INDEX_V_MSG instead.
 */
#define OV_ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("{}\n\tCondition \"{}\" is true. Returning: {}", (m_msg), _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_CONTINUE_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop continues.
 */
#define OV_ERR_CONTINUE(m_cond) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("Condition \"{}\" is true. Continuing.", _OV_STR(m_param)); \
		continue; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop continues.
 */
#define OV_ERR_CONTINUE_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("{}\n\tCondition \"{}\" is true. Continuing.", (m_msg), _OV_STR(m_param)); \
		continue; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_BREAK_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop breaks.
 */
#define OV_ERR_BREAK(m_cond) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("Condition \"{}\" is true. Breaking.", _OV_STR(m_param)); \
		break; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop breaks.
 */
#define OV_ERR_BREAK_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_ERROR("{}\n\tCondition \"{}\" is true. Breaking.", (m_msg), _OV_STR(m_param)); \
		break; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_MSG` or `OV_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the application crashes.
 */
#define OV_CRASH_COND(m_cond) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_CRITICAL("Condition \"{}\" is true.", _OV_STR(m_param)); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_MSG` or `OV_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the application crashes.
 */
#define OV_CRASH_COND_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		SPDLOG_CRITICAL("{}\n\tCondition \"{}\" is true.", (m_msg), _OV_STR(m_param)); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

// Generic error macros.

/**
 * Try using `OV_ERR_FAIL_COND_MSG` or `OV_ERR_FAIL_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns.
 */
#define OV_ERR_FAIL() \
	if (true) { \
		SPDLOG_ERROR("Method/function failed."); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns.
 */
#define OV_ERR_FAIL_MSG(m_msg) \
	if (true) { \
		SPDLOG_ERROR("{}\n\tMethod/function failed.", (m_msg)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_V_MSG` or `OV_ERR_FAIL_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns `m_retval`.
 */
#define OV_ERR_FAIL_V(m_retval) \
	if (true) { \
		SPDLOG_ERROR("Method/function failed. Returning: {}", _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_V_MSG(m_retval, m_msg) \
	if (true) { \
		SPDLOG_ERROR("{}\n\tMethod/function failed. Returning: {}", (m_msg), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Prints `__VA_ARGS__` once during the application lifetime.
 */
#define OV_ERR_PRINT_ONCE(...) \
	if (true) { \
		static bool warning_shown = false; \
		if (OV_unlikely(!warning_shown)) { \
			warning_shown = true; \
			SPDLOG_ERROR(__VA_ARGS__); \
		} \
	} else \
		((void)0)

// Print warning message macros.

/**
 * Prints `__VA_ARGS__` once during the application lifetime.
 *
 * If warning about deprecated usage, use `OV_WARN_DEPRECATED` or `OV_WARN_DEPRECATED_MSG` instead.
 */
#define OV_WARN_PRINT_ONCE(...) \
	if (true) { \
		static bool warning_shown = false; \
		if (OV_unlikely(!warning_shown)) { \
			warning_shown = true; \
			SPDLOG_WARN(__VA_ARGS__); \
		} \
	} else \
		((void)0)

// Print deprecated warning message macros.

/**
 * Warns that the current function is deprecated.
 */
#define OV_WARN_DEPRECATED OV_WARN_PRINT_ONCE("This method has been deprecated and will be removed in the future.")

/**
 * Warns that the current function is deprecated and prints `m_msg`.
 */
#define OV_WARN_DEPRECATED_MSG(m_msg) \
	OV_WARN_PRINT_ONCE("{}\n\tThis method has been deprecated and will be removed in the future.", (m_msg))

/**
 * Do not use.
 * If the application should never reach this point use OV_CRASH_NOW_MSG(m_msg) to explain why.
 *
 * The application crashes.
 */
#define OV_CRASH_NOW() \
	if (true) { \
		SPDLOG_CRITICAL("Method/function failed."); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)

/**
 * Only use if the application should never reach this point.
 *
 * Prints `m_msg`, and then the application crashes.
 */
#define OV_CRASH_NOW_MSG(m_msg) \
	if (true) { \
		SPDLOG_CRITICAL("{}\n\tMethod/function failed.", (m_msg)); \
		spdlog::shutdown(); \
		OV_GENERATE_TRAP(); \
	} else \
		((void)0)
