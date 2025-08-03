#pragma once

#include <cctype>
#include <functional>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Deque.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"
#include "openvic-simulation/utility/Utility.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

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
		[[nodiscard]] size_t operator()(memory::string const& txt) const {
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

	// Useful for contiguous memory
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using vector_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, std::vector<std::pair<Key, T>, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using deque_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, OpenVic::utility::deque<std::pair<Key, T>, Allocator>, IndexType>;

	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using ordered_map = vector_ordered_map<Key, T, Hash, KeyEqual, RawAllocator, IndexType, Allocator>;

	// Useful for contiguous memory
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using vector_ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, std::vector<Key, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using deque_ordered_set =
		tsl::ordered_set<Key, Hash, KeyEqual, Allocator, OpenVic::utility::deque<Key, Allocator>, IndexType>;

	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using ordered_set = vector_ordered_set<Key, Hash, KeyEqual, RawAllocator, IndexType, Allocator>;

	template<typename T>
	concept IsOrderedMap = utility::is_derived_from_specialization_of<T, tsl::ordered_map>;
	template<typename T>
	concept IsOrderedSet = utility::is_derived_from_specialization_of<T, tsl::ordered_set>;
	template<typename T>
	concept IsVectorOrderedMap = utility::is_derived_from_specialization_of<T, vector_ordered_map>;
	template<typename T>
	concept IsVectorOrderedSet = utility::is_derived_from_specialization_of<T, vector_ordered_set>;
	template<typename T>
	concept IsDequeOrderedMap = utility::is_derived_from_specialization_of<T, deque_ordered_map>;
	template<typename T>
	concept IsDequeOrderedSet = utility::is_derived_from_specialization_of<T, deque_ordered_set>;

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
		[[nodiscard]] constexpr size_t operator()(memory::string const& txt) const {
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
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_map = vector_ordered_map<
		Key, T, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_map = deque_ordered_map<
		Key, T, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_map = case_insensitive_vector_ordered_map<Key, T, RawAllocator, IndexType, Allocator>;

	// Useful for contiguous memory
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_set = vector_ordered_set<
		Key, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_set =
		deque_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_set = case_insensitive_vector_ordered_set<Key, RawAllocator, IndexType, Allocator>;

	template<typename Case>
	concept StringMapCase = requires(std::string_view identifier) {
		{ typename Case::hash {}(identifier) } -> std::same_as<std::size_t>;
		{ typename Case::equal {}(identifier, identifier) } -> std::same_as<bool>;
	};
	struct StringMapCaseSensitive {
		using hash = container_hash<memory::string>;
		using equal = std::equal_to<>;
	};
	struct StringMapCaseInsensitive {
		using hash = case_insensitive_string_hash;
		using equal = case_insensitive_string_equal;
	};

	/* Intermediate struct that "remembers" Case, instead of just decomposing it into its hash and equal components,
	 * needed so that templates can deduce the Case with which a type was defined. */
	template<template<typename...> typename Container, StringMapCase Case, typename... Args>
	struct template_case_container_t : Container<Args..., typename Case::hash, typename Case::equal> {
		using container_t = Container<Args..., typename Case::hash, typename Case::equal>;
		using container_t::container_t;

		using case_t = Case;
	};

	/* Template for map with string keys, supporting search by string_view without creating an intermediate string. */
	template<typename T, StringMapCase Case>
	using template_string_map_t = template_case_container_t<ordered_map, Case, memory::string, T>;

	template<typename T>
	using string_map_t = template_string_map_t<T, StringMapCaseSensitive>;
	template<typename T>
	using case_insensitive_string_map_t = template_string_map_t<T, StringMapCaseInsensitive>;

	/* Template for set with string elements, supporting search by string_view without creating an intermediate string. */
	template<StringMapCase Case>
	using template_string_set_t = template_case_container_t<ordered_set, Case, memory::string>;

	using string_set_t = template_string_set_t<StringMapCaseSensitive>;
	using case_insensitive_string_set_t = template_string_set_t<StringMapCaseInsensitive>;
}
