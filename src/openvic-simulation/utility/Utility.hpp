#pragma once

#include <climits>
#include <cmath>
#include <concepts>
#include <functional>
#include <memory>
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

#ifndef OV_NO_UNIQUE_ADDRESS
#if __has_cpp_attribute(msvc::no_unique_address)
#define OV_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __has_cpp_attribute(no_unique_address)
#define OV_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define OV_NO_UNIQUE_ADDRESS
#endif
#endif // OV_NO_UNIQUE_ADDRESS

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

	template<typename T>
	concept boolean_convertible = std::convertible_to<T, bool>;

	template<typename T>
	concept boolean_testable = boolean_convertible<T> && requires(T&& __t) {
		{ !static_cast<T&&>(__t) } -> boolean_convertible;
	};

	[[nodiscard]] inline constexpr auto three_way(auto&& left, auto&& right)
		requires requires {
			{ left < right } -> boolean_testable;
			{ right < left } -> boolean_testable;
		} {
		// This is Apple's fault again
		#if __cpp_lib_three_way_comparison >= 201907L
		if constexpr (std::three_way_comparable_with<std::decay_t<decltype(left)>, std::decay_t<decltype(right)>>) {
			return left <=> right;
		} else
		#endif
		{
			if (left < right) {
				return std::weak_ordering::less;
			} else if (right < left) {
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

	template<typename Allocator, typename T, typename Value = std::remove_cvref_t<typename Allocator::value_type>>
	concept move_insertable_allocator_for =
		requires(Allocator& alloc, Value* value, T move) { std::allocator_traits<Allocator>::construct(alloc, value, move); };

	template<typename Allocator>
	struct enable_copy_insertable : std::false_type {};

	template<typename Allocator>
	concept copy_insertable_allocator = enable_copy_insertable<Allocator>::value ||
		move_insertable_allocator_for<Allocator, typename Allocator::value_type const&>;

	template<typename T>
	struct enable_copy_insertable<std::allocator<T>> : std::is_copy_constructible<T> {};

	template<typename Allocator>
	struct enable_move_insertable : std::false_type {};

	template<typename Allocator>
	concept move_insertable_allocator =
		enable_move_insertable<Allocator>::value || move_insertable_allocator_for<Allocator, typename Allocator::value_type>;

	template<typename T>
	struct enable_move_insertable<std::allocator<T>> : std::is_move_constructible<T> {};

	template<typename InputIt, typename Sentinel, typename ForwardIt, typename Allocator>
	constexpr ForwardIt uninitialized_copy(InputIt first, Sentinel last, ForwardIt result, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_copy(first, last, result);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; first != last; ++first, (void)++result) {
				traits::construct(alloc, std::addressof(*result), *first);
			}
			return result;
		}
	}

	template<typename InputIt, typename Sentinel, typename ForwardIt, typename Allocator>
	constexpr ForwardIt uninitialized_move(InputIt first, Sentinel last, ForwardIt result, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_move(first, last, result);
		} else {
			return uninitialized_copy(std::make_move_iterator(first), std::make_move_iterator(last), result, alloc);
		}
	}

	template<typename ForwardIt, typename Size, typename T, typename Allocator>
	constexpr ForwardIt uninitialized_fill_n(ForwardIt first, Size n, T const& value, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_fill_n(first, n, value);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; n > 0; --n, ++first) {
				traits::construct(alloc, std::addressof(*first), value);
			}
			return first;
		}
	}

	template<typename ForwardIt, typename Size, typename T, typename Allocator>
	constexpr ForwardIt uninitialized_default_n(ForwardIt first, Size n, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_value_construct_n(first, n);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; n > 0; --n, ++first) {
				traits::construct(alloc, std::addressof(*first));
			}
			return first;
		}
	}

	template<typename ForwardIt, typename Allocator>
	constexpr inline void destroy(ForwardIt first, ForwardIt last, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			std::destroy(first, last);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; first != last; ++first) {
				traits::destroy(alloc, std::addressof(*first));
			}
		}
	}

	template<typename T>
	struct is_trivially_relocatable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

	template<typename T>
	static constexpr bool is_trivially_relocatable_v = is_trivially_relocatable<T>::value;
}

namespace OpenVic::cow {
	template<typename T>
	T const& read(T const& v) {
		return v;
	}

	template<typename T>
	T& write(T& v) {
		return v;
	}
}
