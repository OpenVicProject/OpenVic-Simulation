#pragma once

#include <climits>
#include <functional>
#include <type_traits>

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

namespace OpenVic::utility {
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

	template<class T>
	inline constexpr void hash_combine(std::size_t& s, T const& v) {
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	template<size_t Shift, class T>
	inline constexpr void hash_combine_index(std::size_t& s, T const& v) {
		std::hash<T> h;
		if constexpr (Shift == 0) {
			s = h(v);
		} else {
			s ^= h(v) << Shift;
		}
	}

	template<class T, typename... Args>
	constexpr void perfect_hash(std::size_t& s, T&& v, Args&&... args) {
		static_assert(
			sizeof(T) + (sizeof(Args) + ...) <= sizeof(std::size_t), "Perfect hashes must be able to fit into size_t"
		);
		std::hash<T> h;
		if constexpr (sizeof...(args) == 0) {
			s = h(v);
		} else {
			const std::tuple arg_tuple { args... };
			s = h(v) << (sizeof(T) * CHAR_BIT);
			(
				[&] {
					// If args is not last pointer of args
					if (static_cast<void const*>(&(std::get<sizeof...(args) - 1>(arg_tuple))) !=
						static_cast<void const*>(&args)) {
						s <<= sizeof(Args) * CHAR_BIT;
					}
					s |= std::hash<Args> {}(args);
				}(),
				...
			);
		}
	}

	template<typename T, template<typename...> class Z>
	struct is_specialization_of : std::false_type {};

	template<typename... Args, template<typename...> class Z>
	struct is_specialization_of<Z<Args...>, Z> : std::true_type {};

	template<typename T, template<typename...> class Z>
	inline constexpr bool is_specialization_of_v = is_specialization_of<T, Z>::value;

	template <template<typename...> class Template, typename... Args>
	void _derived_from_specialization_impl(Template<Args...> const&);

	template <typename T, template<typename...> class Template>
	concept is_derived_from_specialization_of = requires(T const& t) {
		_derived_from_specialization_impl<Template>(t);
	};

	inline constexpr auto three_way(auto&& left, auto&& right) {
		// This is Apple's fault again
		#if __cpp_lib_three_way_comparison >= 201907L
		if constexpr (std::three_way_comparable_with<std::decay_t<decltype(left)>, std::decay_t<decltype(right)>>) {
			return left <=> right;
		} else
		#endif
		{
			if (left < right) {
				return std::weak_ordering::less;
			} else if (left > right) {
				return std::weak_ordering::greater;
			} else {
				return std::weak_ordering::equivalent;
			}
		}
	};

	template<typename T, typename T2>
	concept not_same_as = !std::same_as<T, T2>;
}
