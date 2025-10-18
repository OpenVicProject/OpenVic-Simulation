#pragma once

#include "openvic-simulation/utility/Logger.hpp" // IWYU pragma: keep for macros
#include "openvic-simulation/utility/Utility.hpp"

/**
 * Try using `OV_ERR_FAIL_COND_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns.
 */
#define OV_ERR_FAIL_MSG(m_msg) \
	if (true) { \
		::OpenVic::Logger::error("Method/function failed. ", m_msg); \
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
		::OpenVic::Logger::error("Method/function failed. Returning: " _OV_STR(m_retval)); \
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
		::OpenVic::Logger::error("Method/function failed. Returning: " _OV_STR(m_retval), " ", m_msg); \
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
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true."); \
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
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true. ", m_msg); \
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
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true. Returning: " _OV_STR(m_retval)); \
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
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true. Returning: " _OV_STR(m_retval) " ", m_msg); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns.
 */
#define OV_ERR_FAIL_INDEX(m_index, m_size) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::Logger::error( \
			"Index " _OV_STR(m_index) " = ", m_index, " is out of bounds (" _OV_STR(m_size) " = ", m_size, ")." \
		); \
		return; \
	} else \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns.
 */
#define OV_ERR_FAIL_INDEX_MSG(m_index, m_size, m_msg) \
	if (OV_unlikely((m_index) < 0 || (m_index) >= (m_size))) { \
		::OpenVic::Logger::error( \
			"Index " _OV_STR(m_index) " = ", m_index, " is out of bounds (" _OV_STR(m_size) " = ", m_size, "). ", m_msg \
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
		::OpenVic::Logger::error( \
			"Index " _OV_STR(m_index) " = ", m_index, " is out of bounds (" _OV_STR(m_size) " = ", m_size, \
			"). Returning: " _OV_STR(m_retval) \
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
		::OpenVic::Logger::error( \
			"Index " _OV_STR(m_index) " = ", m_index, " is out of bounds (" _OV_STR(m_size) " = ", m_size, \
			"). Returning: " _OV_STR(m_retval) " ", m_msg \
		); \
		return m_retval; \
	} else \
		((void)0)

#define OV_ERR_CONTINUE(m_cond) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true. Continuing."); \
		continue; \
	} else \
		((void)0)

#define OV_ERR_BREAK(m_cond) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true. Breaking."); \
		break; \
	} else \
		((void)0)

/**
 * Try using `OV_ERR_FAIL_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns.
 */
#define OV_ERR_FAIL_NULL(m_param) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::Logger::error("Parameter \"" _OV_STR(m_param) "\" is null."); \
		return; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 */
#define OV_ERR_FAIL_NULL_MSG(m_param, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::Logger::error("Parameter \"" _OV_STR(m_param) "\" is null. ", m_msg); \
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
		::OpenVic::Logger::error("Parameter \"" _OV_STR(m_param) "\" is null."); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_NULL_V_MSG(m_param, m_retval, m_msg) \
	if (OV_unlikely(m_param == nullptr)) { \
		::OpenVic::Logger::error("Parameter \"" _OV_STR(m_param) "\" is null. ", m_msg); \
		return m_retval; \
	} else \
		((void)0)
