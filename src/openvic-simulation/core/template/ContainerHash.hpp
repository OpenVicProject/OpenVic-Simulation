#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/string/Utility.hpp"

namespace OpenVic {
	struct ordered_container_string_hash {
		using is_transparent = void;
		[[nodiscard]] std::size_t operator()(char const* txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] std::size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] std::size_t operator()(std::string const& txt) const {
			return std::hash<std::string> {}(txt);
		}
		[[nodiscard]] std::size_t operator()(memory::string const& txt) const {
			return std::hash<std::string_view> {}(txt);
		}
	};

	template<typename T>
	struct container_hash : std::hash<T> {};

	template<>
	struct container_hash<std::string> : ordered_container_string_hash {};
	template<>
	struct container_hash<memory::string> : ordered_container_string_hash {};
	template<>
	struct container_hash<std::string_view> : ordered_container_string_hash {};
	template<>
	struct container_hash<char const*> : ordered_container_string_hash {};
	template<typename T>
	struct container_hash<T*> : std::hash<T const*> {};


	/* Case-Insensitive Containers */
	struct case_insensitive_string_hash {
		using is_transparent = void;

	private:
		/* Based on the byte array hashing functions in MSVC's <type_traits>. */
		[[nodiscard]] static constexpr size_t _hash_bytes_case_insensitive(char const* first, size_t count) {
			constexpr size_t _offset_basis = 14695981039346656037ULL;
			constexpr size_t _prime = 1099511628211ULL;
			size_t hash = _offset_basis;
			for (size_t i = 0; i < count; ++i) {
				hash ^= static_cast<size_t>(std::tolower(static_cast<unsigned char>(first[i])));
				hash *= _prime;
			}
			return hash;
		}

	public:
		[[nodiscard]] constexpr size_t operator()(char const* txt) const {
			return operator()(std::string_view { txt });
		}
		[[nodiscard]] constexpr size_t operator()(std::string_view txt) const {
			return _hash_bytes_case_insensitive(txt.data(), txt.length());
		}
		[[nodiscard]] constexpr size_t operator()(std::string const& txt) const {
			return _hash_bytes_case_insensitive(txt.data(), txt.length());
		}
		[[nodiscard]] constexpr size_t operator()(memory::string const& txt) const {
			return _hash_bytes_case_insensitive(txt.data(), txt.length());
		}
	};

	struct case_insensitive_string_equal {
		using is_transparent = void;

		[[nodiscard]] constexpr bool operator()(std::string_view const& lhs, std::string_view const& rhs) const {
			return strings_equal_case_insensitive(lhs, rhs);
		}
	};

	struct StringMapCaseSensitive {
		using hash = container_hash<memory::string>;
		using equal = std::equal_to<>;
	};
	struct StringMapCaseInsensitive {
		using hash = case_insensitive_string_hash;
		using equal = case_insensitive_string_equal;
	};
}
