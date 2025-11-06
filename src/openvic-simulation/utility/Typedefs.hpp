#pragma once

#if defined(__GNUC__)
#define OV_likely(x) __builtin_expect(!!(x), 1)
#define OV_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define OV_likely(x) x
#define OV_unlikely(x) x
#endif

// Turn argument to string constant:
// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html#Stringizing
#ifndef _OV_STR
#define _OV_STR(m_x) #m_x
#define _OV_MKSTR(m_x) _OV_STR(m_x)
#endif

#ifndef OV_ALWAYS_INLINE
#if defined(__GNUC__)
#define OV_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define OV_ALWAYS_INLINE __forceinline
#else
#define OV_ALWAYS_INLINE inline
#endif // defined(__GNUC__)
#endif // OV_ALWAYS_INLINE

#ifndef OV_SPEED_INLINE
#if defined(DEV_ENABLED) || defined(OPT_SIZE_ENABLED)
#define OV_SPEED_INLINE inline
#else
#define OV_SPEED_INLINE OV_ALWAYS_INLINE
#endif
#endif // OV_SPEED_INLINE

#ifndef OV_NO_UNIQUE_ADDRESS
#if __has_cpp_attribute(msvc::no_unique_address)
#define OV_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
#define OV_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define OV_NO_UNIQUE_ADDRESS
#endif
#endif // OV_NO_UNIQUE_ADDRESS

namespace OpenVic {
	[[noreturn]] inline void unreachable() {
		// Uses compiler specific extensions if possible.
		// Even if no extension is used, undefined behavior is still raised by
		// an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
		__builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
		__assume(false);
#endif
	}

	namespace cow {
		template<typename T>
		T const& read(T const& v) {
			return v;
		}

		template<typename T>
		T& write(T& v) {
			return v;
		}
	}
}
