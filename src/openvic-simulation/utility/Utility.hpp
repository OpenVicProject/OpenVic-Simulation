#pragma once

#include <climits>
#include <functional>
#include <type_traits>

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
	constexpr inline void hash_combine(std::size_t& s, const T& v) {
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	template<size_t Shift, class T>
	constexpr inline void hash_combine_index(std::size_t& s, const T& v) {
		std::hash<T> h;
		if constexpr(Shift == 0) {
			s = h(v);
		} else {
			s ^= h(v) << Shift;
		}
	}

	template<class T, typename ...Args>
	constexpr void perfect_hash(std::size_t& s, T&& v, Args&&... args) {
		static_assert(sizeof(T) + (sizeof(Args) + ...) <= sizeof(std::size_t), "Perfect hashes must be able to fit into size_t");
		std::hash<T> h;
		if constexpr(sizeof...(args) == 0) {
			s = h(v);
		} else {
			const std::tuple arg_tuple { args... };
			s = h(v) << (sizeof(T) * CHAR_BIT);
			([&]{
				// If args is not last pointer of args
				if (static_cast<const void*>(&(std::get<sizeof...(args) - 1>(arg_tuple))) != static_cast<const void*>(&args)) {
					s <<= sizeof(Args) * CHAR_BIT;
				}
				s |= std::hash<Args>{}(args);
			}(), ...);
		}
	}

	template<typename T, template<typename...> class Z>
	struct is_specialization_of : std::false_type {};

	template<typename... Args, template<typename...> class Z>
	struct is_specialization_of<Z<Args...>, Z> : std::true_type {};

	template<typename T, template<typename...> class Z>
	inline constexpr bool is_specialization_of_v = is_specialization_of<T, Z>::value;
}
