#pragma once

#include <bit> // IWYU pragma: keep for use of cross-library reference for standard library macro definitions

/// Adds macros for memory hardening asserts:
// OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, MSG)
// OV_HARDEN_ASSERT_NONEMPTY_RANGE(BEGIN, END, FUNC_NAME) - requires FUNC_NAME to be string literal
// OV_HARDEN_ASSERT_ACCESS(INDEX, FUNC_NAME) - requires `this->size()` is a valid call and FUNC_NAME to be string literal
// OV_HARDEN_ASSERT_NONEMPTY(FUNC_NAME) - requires `!this->empty()` is a valid call and FUNC_NAME to be string literal
// OV_HARDEN_ASSERT_VALID_ITERATOR(IT, FUNC_NAME) - requires FUNC_NAME to be string literal
//
// Adds macros for aborting/throwing for strict container errors:
// OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE)
// OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) - requires CLASS_NAME and FUNC_NAME to be string literals

#ifdef __GLIBCXX__
#include <debug/assertions.h>

#define OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, MSG) __glibcxx_assert(BEGIN <= END)
#define OV_HARDEN_ASSERT_NONEMPTY_RANGE(BEGIN, END, FUNC_NAME) __glibcxx_requires_non_empty_range(BEGIN, END)

#define OV_HARDEN_ASSERT_ACCESS(INDEX, FUNC_NAME) __glibcxx_requires_subscript(INDEX)
#define OV_HARDEN_ASSERT_NONEMPTY(FUNC_NAME) __glibcxx_requires_nonempty()
#define OV_HARDEN_ASSERT_VALID_ITERATOR(IT, FUNC_NAME) \
	OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(IT, end(), FUNC_NAME " called with a non-dereferenceable iterator")

#elif defined(_LIBCPP_VERSION)
#include <__assert>

#define OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, MSG) _LIBCPP_ASSERT_VALID_INPUT_RANGE(BEGIN <= END, MSG)
#define OV_HARDEN_ASSERT_NONEMPTY_RANGE(BEGIN, END, FUNC_NAME) \
	OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, FUNC_NAME " called with an invalid range")

#define OV_HARDEN_ASSERT_ACCESS(INDEX, FUNC_NAME) \
	_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(INDEX < size(), FUNC_NAME " index out of bounds")
#define OV_HARDEN_ASSERT_NONEMPTY(FUNC_NAME) \
	_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(!empty(), FUNC_NAME " called on an empty container")
#define OV_HARDEN_ASSERT_VALID_ITERATOR(IT, FUNC_NAME) \
	_LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(IT != end(), FUNC_NAME " called with a non-dereferenceable iterator")

#elif defined(_MSVC_STL_VERSION) && _MSVC_STL_HARDENING == 1
#include <yvals.h>

#define OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, MSG) _STL_VERIFY(BEGIN <= END, MSG)
#define OV_HARDEN_ASSERT_NONEMPTY_RANGE(BEGIN, END, FUNC_NAME) \
	OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, FUNC_NAME " called with an invalid range")

#define OV_HARDEN_ASSERT_ACCESS(INDEX, FUNC_NAME) _STL_VERIFY(INDEX < size(), FUNC_NAME " index out of bounds")
#define OV_HARDEN_ASSERT_NONEMPTY(FUNC_NAME) _STL_VERIFY(!empty(), FUNC_NAME " called on an empty container")
#define OV_HARDEN_ASSERT_VALID_ITERATOR(IT, FUNC_NAME) \
	OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(IT, end(), FUNC_NAME " called with a non-dereferenceable iterator")

#else
#define OV_HARDEN_ASSERT_VALID_RANGE_MESSAGE(BEGIN, END, MSG)
#define OV_HARDEN_ASSERT_NONEMPTY_RANGE(BEGIN, END, FUNC_NAME)
#define OV_HARDEN_ASSERT_ACCESS(INDEX, FUNC_NAME)
#define OV_HARDEN_ASSERT_NONEMPTY(FUNC_NAME)
#define OV_HARDEN_ASSERT_VALID_ITERATOR(IT, FUNC_NAME)

// clang-format off
#   if defined(_GLIBCXX_ASSERTIONS)
	|| (defined(_LIBCPP_HARDENING_MODE) && _LIBCPP_HARDENING_MODE == _LIBCPP_HARDENING_MODE_FAST)
	|| _MSVC_STL_HARDENING == 1
#warning "Unsupported standard library for memory hardening, hardening asserts will be ignored."
#   endif
// clang-format on

#endif

#ifdef __GLIBCXX__
#if __has_include(<bits/functexcept.h>)
#include <bits/functexcept.h>
#elif __has_include(<bits/stdexcept_throw.h>)
#include <bits/stdexcept_throw.h>
#else
#include <cstdlib>
#warning "Unknown GLIBCXX library version"
#define OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE) std::abort()
#define OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) std::abort()
#endif

#ifndef OV_THROW_OUT_OF_RANGE
#define OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE) \
	std::__throw_out_of_range_fmt( \
		__N("%s::%s: %s (which is %zu) >= this->size() (which is %zu)"), CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE \
	)
#endif

#ifndef OV_THROW_LENGTH_ERROR
#define OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) std::__throw_length_error(__N(CLASS_NAME "::" FUNC_NAME))
#endif

#elif defined(_LIBCPP_VERSION) && __has_include(<stdexcept>)
#include <stdexcept>

#define OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE) std::__throw_out_of_range(CLASS_NAME)
#define OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) std::__throw_length_error(CLASS_NAME)

#elif defined(_MSVC_STL_VERSION) && __has_include(<xutility>)
#include <xutility>

#define OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE) \
	std::_Xout_of_range("invalid " CLASS_NAME " subscript")
#define OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) std::_Xlength_error(CLASS_NAME " too long")
#endif

#ifndef OV_THROW_OUT_OF_RANGE
#include <cstdlib>
#define OV_THROW_OUT_OF_RANGE(CLASS_NAME, FUNC_NAME, VAR_NAME, VAR, SIZE) std::abort()
#endif

#ifndef OV_THROW_LENGTH_ERROR
#include <cstdlib>
#define OV_THROW_LENGTH_ERROR(CLASS_NAME, FUNC_NAME) std::abort()
#endif
