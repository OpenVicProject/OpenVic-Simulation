#pragma once

#include <snitch/snitch_macros_check_base.hpp>
#include <snitch/snitch_macros_utility.hpp>
#include <snitch/snitch_macros_warnings.hpp>

#define _EXPR(TYPE, EXPECTED, ASSIGN_VALUE, ...) \
	auto SNITCH_CURRENT_EXPRESSION = \
		(snitch::impl::expression_extractor<EXPECTED> { TYPE, #__VA_ARGS__ } <= __VA_ARGS__).to_expression(); \
	ASSIGN_VALUE = SNITCH_CURRENT_EXPRESSION.success;

#define _REQUIRE_IMPL(CHECK, EXPECTED, MAYBE_ABORT, ASSIGN_VALUE, ...) \
	do { \
		auto SNITCH_CURRENT_CHECK = SNITCH_NEW_CHECK; \
		SNITCH_WARNING_PUSH \
		SNITCH_WARNING_DISABLE_PARENTHESES \
		SNITCH_WARNING_DISABLE_CONSTANT_COMPARISON \
		if constexpr (SNITCH_IS_DECOMPOSABLE(__VA_ARGS__)) { \
			_EXPR(CHECK, EXPECTED, ASSIGN_VALUE, __VA_ARGS__); \
			SNITCH_REPORT_EXPRESSION(MAYBE_ABORT); \
		} else { \
			ASSIGN_VALUE = static_cast<bool>(__VA_ARGS__); \
			const auto SNITCH_CURRENT_EXPRESSION = \
				snitch::impl::expression { CHECK, #__VA_ARGS__, {}, ASSIGN_VALUE == EXPECTED }; \
			SNITCH_REPORT_EXPRESSION(MAYBE_ABORT); \
		} \
		SNITCH_WARNING_POP \
	} while (0)

// clang-format off
#define _OVSIM_REQUIRE(NAME, ASSIGN_VALUE, ...)       _REQUIRE_IMPL("REQUIRE" NAME,       true,  SNITCH_TESTING_ABORT, ASSIGN_VALUE, __VA_ARGS__)
#define _OVSIM_CHECK(NAME, ASSIGN_VALUE, ...)         _REQUIRE_IMPL("CHECK" NAME,         true,  (void)0,              ASSIGN_VALUE, __VA_ARGS__)
#define _OVSIM_REQUIRE_FALSE(NAME, ASSIGN_VALUE, ...) _REQUIRE_IMPL("REQUIRE_FALSE" NAME, false, SNITCH_TESTING_ABORT, ASSIGN_VALUE, __VA_ARGS__)
#define _OVSIM_CHECK_FALSE(NAME, ASSIGN_VALUE, ...)   _REQUIRE_IMPL("CHECK_FALSE" NAME,   false, (void)0,              ASSIGN_VALUE, __VA_ARGS__)
// clang-format on

#define _OVSIM_CHECK_IF(NAME, ...) \
	if (bool SNITCH_MACRO_CONCAT(result_, __LINE__) = false; [&] { _OVSIM_CHECK(NAME, (SNITCH_MACRO_CONCAT(result_, __LINE__)), __VA_ARGS__); }(), (SNITCH_MACRO_CONCAT(result_, __LINE__)))

#define _OVSIM_CHECK_FALSE_IF(NAME, ...) \
	if (bool SNITCH_MACRO_CONCAT(result_, __LINE__) = false; [&] { _OVSIM_CHECK_FALSE(NAME, (SNITCH_MACRO_CONCAT(result_, __LINE__)), __VA_ARGS__); }(), (!SNITCH_MACRO_CONCAT(result_, __LINE__)))

#define CHECK_IF(...) _OVSIM_CHECK_IF("_IF", __VA_ARGS__)

#define CHECK_FALSE_IF(...) _OVSIM_CHECK_FALSE_IF("_IF", __VA_ARGS__)

#define CHECK_RETURN_BOOL(...) \
	_OVSIM_CHECK_IF("_RETURN_BOOL", __VA_ARGS__) { \
		return true; \
	} \
	else return false;

#define CHECK_FALSE_RETURN_BOOL(...) \
	_OVSIM_CHECK_FALSE_IF("_RETURN_BOOL", __VA_ARGS__) { \
		return true; \
	} \
	else return false;

#define CHECK_OR_RETURN(...) \
	_OVSIM_CHECK_IF("_OR_RETURN", __VA_ARGS__); \
	else return
#define CHECK_FALSE_OR_RETURN(...) \
	_OVSIM_CHECK_FALSE_IF("_OR_RETURN", __VA_ARGS__); \
	else return

#define CHECK_OR_CONTINUE(...) \
	_OVSIM_CHECK_IF("_OR_CONTINUE", __VA_ARGS__); \
	else continue
#define CHECK_FALSE_OR_CONTINUE(...) \
	_OVSIM_CHECK_FALSE_IF("_OR_CONTINUE", __VA_ARGS__); \
	else continue
