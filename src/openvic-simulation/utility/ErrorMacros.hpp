#pragma once

#include "openvic-simulation/utility/Logger.hpp" // IWYU pragma: keep
#include "openvic-simulation/utility/Utility.hpp"

/**
 * Try using `ERR_FAIL_COND_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_MSG instead.
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
 * If checking for null use ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_MSG instead.
 */
#define OV_ERR_FAIL_COND_MSG(m_cond, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::Logger::error("Condition \"" _OV_STR(m_cond) "\" is true.", m_msg); \
		return; \
	} else \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns `m_retval`.
 */
#define OV_ERR_FAIL_COND_V(m_cond, m_retval) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::Logger::error( \
			"Condition \"" _OV_STR(m_cond) "\" is true. Returning: " _OV_STR(m_retval) \
		); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns `m_retval`.
 *
 * If checking for null use ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 */
#define OV_ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg) \
	if (OV_unlikely(m_cond)) { \
		::OpenVic::Logger::error( \
			"Condition \"" _OV_STR(m_cond) "\" is true. Returning: " _OV_STR(m_retval), m_msg \
		); \
		return m_retval; \
	} else \
		((void)0)
