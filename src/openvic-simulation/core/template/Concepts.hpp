#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string_view>
#include <type_traits>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
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

	template<template<typename...> class Template, typename... Args>
	void _derived_from_specialization_impl(Template<Args...> const&);

	template<typename T, template<typename...> class Template>
	concept derived_from_specialization_of = requires(T const& t) { _derived_from_specialization_impl<Template>(t); };

	template<typename T>
	concept boolean_convertible = std::convertible_to<T, bool>;

	template<typename T>
	concept boolean_testable = boolean_convertible<T> && requires(T&& __t) {
		{ !static_cast<T&&>(__t) } -> boolean_convertible;
	};

	template<typename T, typename T2>
	concept not_same_as = !std::same_as<T, T2>;

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

	template<typename T>
	struct is_trivially_relocatable : std::bool_constant<std::is_trivially_copyable_v<T>> {};

	template<typename T>
	static constexpr bool is_trivially_relocatable_v = is_trivially_relocatable<T>::value;

	template<typename T>
	concept trivially_relocatable = is_trivially_relocatable_v<T>;

	template<typename T>
	concept unique_file_key = requires(T const& unique_key, std::string_view path) {
		requires std::same_as<std::remove_cvref_t<decltype(unique_key(path))>, std::string_view>;
	};

	// This adds to capacity rather than size so that it can be used multiple times in a row.
	// If it added to size, it would only reserve enough for max(arguments...)
	template<typename T>
	concept reservable = requires(T& t, std::size_t size) {
		{ t.capacity() } -> std::same_as<std::size_t>;
		t.reserve(size);
	};

	template<typename T, typename... U>
	concept any_of = (std::same_as<T, U> || ...);

	template<typename T>
	concept either_char_type = any_of<T, char, wchar_t>;

	template<typename T>
	concept has_data = requires(T t) {
		{ t.data() } -> std::convertible_to<typename T::value_type const*>;
	};

	template<typename T>
	concept has_get_identifier = requires(T const& t) {
		{ t.get_identifier() } -> std::same_as<std::string_view>;
	};

	template<typename T>
	concept has_get_name = requires(T const& t) {
		{ t.get_name() } -> std::same_as<std::string_view>;
	};

	template<typename T>
	concept has_index = requires { typename T::index_t; } &&
		derived_from_specialization_of<typename T::index_t, type_safe::strong_typedef> && requires {
			static_cast<std::size_t>(
				static_cast<type_safe::underlying_type<decltype(std::declval<T>().index)>>(std::declval<T>().index)
			);
		};

	// helper to avoid error 'index_t': is not a member of T
	template<typename T, typename = void>
	struct get_index_t {
		using type = std::size_t;
	};

	template<has_index T>
	struct get_index_t<T, std::void_t<typename T::index_t>> {
		using type = typename T::index_t;
	};

	template<typename T>
	constexpr std::size_t get_index_as_size_t(const T typed_index) {
		if constexpr (std::is_same_v<T, std::size_t>) {
			return typed_index;
		} else {
			return type_safe::get(typed_index);
		}
	}

	template<typename Case>
	concept string_map_case = requires(std::string_view identifier) {
		{ typename Case::hash {}(identifier) } -> std::same_as<std::size_t>;
		{ typename Case::equal {}(identifier, identifier) } -> std::same_as<bool>;
	};

	template<typename T>
	concept integral_max_size_4 = std::integral<T> && sizeof(T) <= 4;

	template<typename T>
	concept unary_negatable = requires(T const& a) {
		{ -a } -> std::same_as<T>;
	};

	// Concept: pre_incrementable
	// Describes types that support the pre-increment operator (++a).
	// The result of ++a must be a reference to T.
	template<typename T>
	concept pre_incrementable = requires(T a) {
		{ ++a } -> std::same_as<T&>;
	};

	// Concept: post_incrementable
	// Describes types that support the post-increment operator (a++).
	// The result of a++ must be a value of T.
	template<typename T>
	concept post_incrementable = requires(T a) {
		{ a++ } -> std::same_as<T>;
	};

	// Concept: pre_decrementable
	// Describes types that support the pre-decrement operator (++a).
	// The result of ++a must be a reference to T.
	template<typename T>
	concept pre_decrementable = requires(T a) {
		{ --a } -> std::same_as<T&>;
	};

	// Concept: post_decrementable
	// Describes types that support the post-decrement operator (a++).
	// The result of a++ must be a value of T.
	template<typename T>
	concept post_decrementable = requires(T a) {
		{ a-- } -> std::same_as<T>;
	};

	template<typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept addable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs + rhs } -> std::convertible_to<Result>;
	};

	template<typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept subtractable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs - rhs } -> std::convertible_to<Result>;
	};

	template<typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept multipliable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs* rhs } -> std::convertible_to<Result>;
	};

	template<typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept divisible = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs / rhs } -> std::convertible_to<Result>;
	};

	template<typename Lhs, typename Rhs = Lhs>
	concept add_assignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs += rhs } -> std::same_as<Lhs&>;
	};

	template<typename Lhs, typename Rhs = Lhs>
	concept subtract_assignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs -= rhs } -> std::same_as<Lhs&>;
	};

	template<typename Lhs, typename Rhs = Lhs>
	concept multiply_assignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs *= rhs } -> std::same_as<Lhs&>;
	};

	template<typename Lhs, typename Rhs = Lhs>
	concept divide_assignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs /= rhs } -> std::same_as<Lhs&>;
	};

	template<typename Lhs, typename A, typename B>
	concept mul_add_assignable = requires(Lhs& lhs, const A a, const B b) {
		{ lhs += a * b } -> std::same_as<Lhs&>;
	};

	template<typename Lhs, typename Rhs = Lhs>
	concept equalable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs == rhs } -> std::convertible_to<bool>;
	};
}
