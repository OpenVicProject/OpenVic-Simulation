#pragma once

#include <cctype>
#include <deque>
#include <functional>
#include <memory>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include "openvic-simulation/utility/StringUtils.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct ordered_container_string_hash {
		using is_transparent = void;
		[[nodiscard]] size_t operator()(char const* txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] size_t operator()(std::string const& txt) const {
			return std::hash<std::string> {}(txt);
		}
	};

	template<typename T>
	struct container_hash : std::hash<T> {};

	template<>
	struct container_hash<std::string> : ordered_container_string_hash {};
	template<>
	struct container_hash<std::string_view> : ordered_container_string_hash {};
	template<>
	struct container_hash<char const*> : ordered_container_string_hash {};

	// Useful for contiguous memory
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using vector_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, std::vector<std::pair<Key, T>, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using deque_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, std::deque<std::pair<Key, T>, Allocator>, IndexType>;

	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using ordered_map = vector_ordered_map<Key, T, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for contiguous memory
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using vector_ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, std::vector<Key, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using deque_ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, std::deque<Key, Allocator>, IndexType>;

	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using ordered_set = vector_ordered_set<Key, Hash, KeyEqual, Allocator, IndexType>;

	template<typename T>
	concept IsOrderedMap = utility::is_specialization_of_v<T, tsl::ordered_map>;
	template<typename T>
	concept IsOrderedSet = utility::is_specialization_of_v<T, tsl::ordered_set>;
	template<typename T>
	concept IsVectorOrderedMap = utility::is_specialization_of_v<T, vector_ordered_map>;
	template<typename T>
	concept IsVectorOrderedSet = utility::is_specialization_of_v<T, vector_ordered_set>;
	template<typename T>
	concept IsDequeOrderedMap = utility::is_specialization_of_v<T, deque_ordered_map>;
	template<typename T>
	concept IsDequeOrderedSet = utility::is_specialization_of_v<T, deque_ordered_set>;

	template<typename T, typename Key, typename Value>
	concept IsOrderedMapOf =
		IsOrderedMap<T> && std::same_as<Key, typename T::key_type> && std::same_as<Value, typename T::mapped_type>;
	template<typename T, typename Key>
	concept IsOrderedSetOf = IsOrderedSet<T> && std::same_as<Key, typename T::key_type>;
	template<typename T, typename Key, typename Value>
	concept IsVectorOrderedMapOf =
		IsVectorOrderedMap<T> && std::same_as<Key, typename T::key_type> && std::same_as<Value, typename T::mapped_type>;
	template<typename T, typename Key>
	concept IsVectorOrderedSetOf = IsVectorOrderedSet<T> && std::same_as<Key, typename T::key_type>;
	template<typename T, typename Key, typename Value>
	concept IsDequeOrderedMapOf =
		IsDequeOrderedMap<T> && std::same_as<Key, typename T::key_type> && std::same_as<Value, typename T::mapped_type>;
	template<typename T, typename Key>
	concept IsDequeOrderedSetOf = IsDequeOrderedSet<T> && std::same_as<Key, typename T::key_type>;

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
	};

	struct case_insensitive_string_equal {
		using is_transparent = void;

		[[nodiscard]] constexpr bool operator()(std::string_view const& lhs, std::string_view const& rhs) const {
			return StringUtils::strings_equal_case_insensitive(lhs, rhs);
		}
	};

	// Useful for contiguous memory
	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_vector_ordered_map =
		vector_ordered_map<Key, T, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_deque_ordered_map =
		deque_ordered_map<Key, T, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_ordered_map = case_insensitive_vector_ordered_map<Key, T, Allocator, IndexType>;

	// Useful for contiguous memory
	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_vector_ordered_set =
		vector_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_deque_ordered_set =
		deque_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_ordered_set = case_insensitive_vector_ordered_set<Key, Allocator, IndexType>;
}
