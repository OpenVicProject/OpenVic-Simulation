#pragma once

#include "openvic-simulation/core/Typedefs.hpp" // IWYU pragma: keep
#include "openvic-simulation/definition/dataloader/Logger.hpp" // IWYU pragma: keep

// Based heavily on https://github.com/godotengine/godot/blob/34d06658a85845111a50db9e485ec4a0701d4298/core/error/error_macros.h

/**
 * Dataloader Error macros.
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
 * issues when expanded e.g. after an `if (cond) OV_DL_ERR_FAIL();` without braces.
 */

// Index out of bounds error macros.
// These macros should be used instead of `OV_DL_ERR_FAIL_COND` for bounds checking.

// Integer index out of bounds error macros.

/**
 * Try using `OV_DL_ERR_FAIL_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns.
 */
#define OV_DL_ERR_FAIL_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns.
 */
#define OV_DL_ERR_FAIL_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_INDEX_V(m_index, m_size, m_retval) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "Index {} = {} is out of bounds ({} = {}). Returning: {}", \
		    _OV_STR(m_index), \
		    (m_index), \
		    _OV_STR(m_size), \
		    (m_size), \
		    _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_INDEX_V_MSG(m_index, m_size, m_retval, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tIndex {} = {} is out of bounds ({} = {}). Returning: {}", \
		    (m_msg), \
		    _OV_STR(m_index), \
		    (m_index), \
		    _OV_STR(m_size), \
		    (m_size), \
		    _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

// Unsigned integer index out of bounds error macros.

/**
 * Try using `OV_DL_ERR_FAIL_UNSIGNED_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns.
 */
#define OV_DL_ERR_FAIL_UNSIGNED_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "Index {} = {} is out of bounds ({} = {}).", _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns.
 */
#define OV_DL_ERR_FAIL_UNSIGNED_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tIndex {} = {} is out of bounds ({} = {}).", (m_msg), _OV_STR(m_index), (m_index), _OV_STR(m_size), (m_size) \
		); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_UNSIGNED_INDEX_V(m_index, m_size, m_retval) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "Index {} = {} is out of bounds ({} = {}). Returning: {}", \
		    _OV_STR(m_index), \
		    (m_index), \
		    _OV_STR(m_size), \
		    (m_size), \
		    _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_UNSIGNED_INDEX_V_MSG(m_index, m_size, m_retval, m_msg) \
	if (OV_unlikely((m_index) >= (m_size))) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tIndex {} = {} is out of bounds ({} = {}). Returning: {}", \
		    (m_msg), \
		    _OV_STR(m_index), \
		    (m_index), \
		    _OV_STR(m_size), \
		    (m_size), \
		    _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

// Null reference error macros.

/**
 * Try using `OV_DL_ERR_FAIL_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns.
 */
#define OV_DL_ERR_FAIL_NULL(m_param) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::dataloader::log::error("Parameter \"{}\" is null.", _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 */
#define OV_DL_ERR_FAIL_NULL_MSG(m_param, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::dataloader::log::error("{}\n\tParameter \"{}\" is null.", (m_msg), _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_NULL_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_NULL_V(m_param, m_retval) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::dataloader::log::error("Parameter \"{}\" is null. Returning: {}", _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_NULL_V_MSG(m_param, m_retval, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tParameter \"{}\" is null. Returning: {}", (m_msg), _OV_STR(m_param), _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_COND_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use OV_DL_ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use OV_DL_ERR_FAIL_INDEX_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns.
 */
#define OV_DL_ERR_FAIL_COND(m_cond) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("Condition \"{}\" is true.", _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns.
 *
 * If checking for null use OV_DL_ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use OV_DL_ERR_FAIL_INDEX_MSG instead.
 */
#define OV_DL_ERR_FAIL_COND_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("{}\n\tCondition \"{}\" is true.", (m_msg), _OV_STR(m_param)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use OV_DL_ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use OV_DL_ERR_FAIL_INDEX_V_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_COND_V(m_cond, m_retval) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("Condition \"{}\" is true. Returning: {}", _OV_STR(m_param), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns `m_retval`.
 *
 * If checking for null use OV_DL_ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use OV_DL_ERR_FAIL_INDEX_V_MSG instead.
 */
#define OV_DL_ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error( \
		    "{}\n\tCondition \"{}\" is true. Returning: {}", (m_msg), _OV_STR(m_param), _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_CONTINUE_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop continues.
 */
#define OV_DL_ERR_CONTINUE(m_cond) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("Condition \"{}\" is true. Continuing.", _OV_STR(m_param)); \
		continue; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop continues.
 */
#define OV_DL_ERR_CONTINUE_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("{}\n\tCondition \"{}\" is true. Continuing.", (m_msg), _OV_STR(m_param)); \
		continue; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_BREAK_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop breaks.
 */
#define OV_DL_ERR_BREAK(m_cond) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("Condition \"{}\" is true. Breaking.", _OV_STR(m_param)); \
		break; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop breaks.
 */
#define OV_DL_ERR_BREAK_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::dataloader::log::error("{}\n\tCondition \"{}\" is true. Breaking.", (m_msg), _OV_STR(m_param)); \
		break; \
	} else \
		((void)0)

// Generic error macros.

/**
 * Try using `OV_DL_ERR_FAIL_COND_MSG` or `OV_DL_ERR_FAIL_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns.
 */
#define OV_DL_ERR_FAIL() \
	if (true) { \
		::OpenVic::dataloader::log::error("Method/function failed."); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_COND_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns.
 */
#define OV_DL_ERR_FAIL_MSG(m_msg) \
	if (true) { \
		::OpenVic::dataloader::log::error("{}\n\tMethod/function failed.", (m_msg)); \
		return; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_COND_V_MSG` or `OV_DL_ERR_FAIL_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_V(m_retval) \
	if (true) { \
		::OpenVic::dataloader::log::error("Method/function failed. Returning: {}", _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_DL_ERR_FAIL_COND_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns `m_retval`.
 */
#define OV_DL_ERR_FAIL_V_MSG(m_retval, m_msg) \
	if (true) { \
		::OpenVic::dataloader::log::error("{}\n\tMethod/function failed. Returning: {}", (m_msg), _OV_STR(m_retval)); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Prints `__VA_ARGS__` once during the application lifetime.
 */
#define OV_DL_ERR_PRINT_ONCE(...) \
	if (true) { \
		static bool warning_shown = false; \
		if (OV_unlikely(!warning_shown)) { \
			warning_shown = true; \
			::OpenVic::dataloader::log::error(__VA_ARGS__); \
		} \
	} else \
		((void)0)

// Print warning message macros.

/**
 * Prints `__VA_ARGS__` once during the application lifetime.
 *
 * If warning about deprecated usage, use `OV_WARN_DEPRECATED` or `OV_WARN_DEPRECATED_MSG` instead.
 */
#define OV_DL_WARN_PRINT_ONCE(...) \
	if (true) { \
		static bool warning_shown = false; \
		if (OV_unlikely(!warning_shown)) { \
			warning_shown = true; \
			::OpenVic::dataloader::log::warn(__VA_ARGS__); \
		} \
	} else \
		((void)0)

// Print deprecated warning message macros.

/**
 * Warns that the current function is deprecated.
 */
#define OV_DL_WARN_DEPRECATED OV_WARN_PRINT_ONCE("This method has been deprecated and will be removed in the future.")

/**
 * Warns that the current function is deprecated and prints `m_msg`.
 */
#define OV_DL_WARN_DEPRECATED_MSG(m_msg) \
	OV_DL_WARN_PRINT_ONCE("{}\n\tThis method has been deprecated and will be removed in the future.", (m_msg))
