#pragma once

#include <concepts>

namespace OpenVic {
	template <typename T>
	concept UnaryNegatable = requires(T const& a) {
		{ -a } -> std::same_as<T>;
	};
	
	// Concept: PreIncrementable
	// Describes types that support the pre-increment operator (++a).
	// The result of ++a must be a reference to T.
	template <typename T>
	concept PreIncrementable = requires(T a) {
		{ ++a } -> std::same_as<T&>;
	};

	// Concept: PostIncrementable
	// Describes types that support the post-increment operator (a++).
	// The result of a++ must be a value of T.
	template <typename T>
	concept PostIncrementable = requires(T a) {
		{ a++ } -> std::same_as<T>;
	};
	
	// Concept: PreDecrementable
	// Describes types that support the pre-decrement operator (++a).
	// The result of ++a must be a reference to T.
	template <typename T>
	concept PreDecrementable = requires(T a) {
		{ --a } -> std::same_as<T&>;
	};

	// Concept: PostDecrementable
	// Describes types that support the post-decrement operator (a++).
	// The result of a++ must be a value of T.
	template <typename T>
	concept PostDecrementable = requires(T a) {
		{ a-- } -> std::same_as<T>;
	};

	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Addable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs + rhs } -> std::convertible_to<Result>;
	};

	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Subtractable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs - rhs } -> std::convertible_to<Result>;
	};

	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Multipliable = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs * rhs } -> std::convertible_to<Result>;
	};

	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Divisible = requires(Lhs const& lhs, Rhs const& rhs) {
		{ lhs / rhs } -> std::convertible_to<Result>;
	};

	template <typename Lhs, typename Rhs = Lhs>
	concept AddAssignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs += rhs } -> std::same_as<Lhs&>;
	};

	template <typename Lhs, typename Rhs = Lhs>
	concept SubtractAssignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs -= rhs } -> std::same_as<Lhs&>;
	};

	template <typename Lhs, typename Rhs = Lhs>
	concept MultiplyAssignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs *= rhs } -> std::same_as<Lhs&>;
	};

	template <typename Lhs, typename Rhs = Lhs>
	concept DivideAssignable = requires(Lhs& lhs, Rhs const& rhs) {
		{ lhs /= rhs } -> std::same_as<Lhs&>;
	};

	template <typename Lhs, typename A, typename B>
	concept MulAddAssignable = requires(Lhs& lhs, const A a, const B b) {
		{ lhs += a * b } -> std::same_as<Lhs&>;
	};
}