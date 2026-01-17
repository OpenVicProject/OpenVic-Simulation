#pragma once

#include <bit> // IWYU pragma: keep for use of cross-library reference for standard library macro definitions

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
#   if defined(_GLIBCXX_ASSERTIONS) || _LIBCPP_HARDENING_MODE == _LIBCPP_HARDENING_MODE_FAST || _MSVC_STL_HARDENING == 1
#warning "Unsupported standard library for memory hardening, hardening asserts will be ignored."
#   endif
// clang-format on

#endif
