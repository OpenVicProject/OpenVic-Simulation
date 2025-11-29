#pragma once

#include <climits>
#include <cstddef>
#include <functional>

namespace OpenVic {
	static constexpr std::size_t MURMUR3_SEED = 0x7F07C65;

	inline constexpr std::size_t hash_murmur3(std::size_t key, std::size_t seed = MURMUR3_SEED) {
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
}
