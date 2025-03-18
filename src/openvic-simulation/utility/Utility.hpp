#pragma once

#include <climits>
#include <cmath>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

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

	static constexpr std::size_t HASH_MURMUR3_SEED = 0x7F07C65;

	inline constexpr std::size_t hash_murmur3(std::size_t key, std::size_t seed = HASH_MURMUR3_SEED) {
		key ^= seed;
		key ^= key >> 33;
		key *= 0xff51afd7ed558ccd;
		key ^= key >> 33;
		key *= 0xc4ceb9fe1a85ec53;
		key ^= key >> 33;
		return key;
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

	template<typename T, template<typename...> class Template>
	concept specialization_of = is_specialization_of_v<T, Template>;

	template<typename T, template<typename...> class Template>
	concept not_specialization_of = !specialization_of<T, Template>;

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

	template<typename T> requires std::integral<T> || std::floating_point<T>
	[[nodiscard]] inline constexpr T abs(T num) {
		if (std::is_constant_evaluated()) {
			return num < 0 ? -num : num;
		} else {
			return std::abs(num);
		}
	}

	template<typename InputIt, typename Pred>
	[[nodiscard]] inline constexpr InputIt find_if_dual_adjacent(InputIt begin, InputIt end, Pred&& predicate) {
		if (end - begin <= 2) {
			return end;
		}
		++begin;
		for (;begin != end - 1; ++begin) {
			if (predicate(*(begin - 1), *begin, *(begin + 1))) {
				return begin;
			}
		}
		return end;
	}

	template<typename ForwardIt, typename Pred>
	[[nodiscard]] inline constexpr ForwardIt remove_if_dual_adjacent(ForwardIt begin, ForwardIt end, Pred&& predicate) {
		begin = find_if_dual_adjacent(begin, end, std::forward<Pred>(predicate));
		if (begin != end) {
			for (ForwardIt it = begin; ++it != end - 1;) {
				if (!predicate(*(it - 1), *it, *(it + 1))) {
					*begin++ = std::move(*it);
				}
			}
			*begin++ = std::move(*(end - 1));
			return begin;
		}
		return end;
	}
}
