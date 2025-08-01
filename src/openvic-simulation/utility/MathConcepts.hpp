#pragma once

#include <concepts>

namespace OpenVic {
	// Concept: UnaryNegatable
	// Describes types that support the unary minus operator (-)
	// The result type of -a must be the same as T.
	template <typename T>
	concept UnaryNegatable = requires(T a) {
		{ -a } -> std::same_as<T>;
	};

	// Concept: Addable
	// Describes types Lhs and Rhs that support the binary addition operator (+)
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs + rhs must be convertible to Result.
	// By default, Result is std::common_type_t<Lhs, Rhs>.
	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Addable = requires(Lhs lhs, Rhs rhs) {
		{ lhs + rhs } -> std::convertible_to<Result>;
	};

	// Concept: Subtractable
	// Describes types Lhs and Rhs that support the binary subtraction operator (-)
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs - rhs must be convertible to Result.
	// By default, Result is std::common_type_t<Lhs, Rhs>.
	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Subtractable = requires(Lhs lhs, Rhs rhs) {
		{ lhs - rhs } -> std::convertible_to<Result>;
	};

	// Concept: Multipliable
	// Describes types Lhs and Rhs that support the binary multiplication operator (*)
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs * rhs must be convertible to Result.
	// By default, Result is std::common_type_t<Lhs, Rhs>.
	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Multipliable = requires(Lhs lhs, Rhs rhs) {
		{ lhs * rhs } -> std::convertible_to<Result>;
	};

	// Concept: Divisible
	// Describes types Lhs and Rhs that support the binary division operator (/)
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs / rhs must be convertible to Result.
	// By default, Result is std::common_type_t<Lhs, Rhs>.
	// Note: This concept only checks for syntactic validity. Division by zero
	// is a runtime error and cannot be checked at compile time with concepts alone.
	template <typename Lhs, typename Rhs = Lhs, typename Result = std::common_type_t<Lhs, Rhs>>
	concept Divisible = requires(Lhs lhs, Rhs rhs) {
		{ lhs / rhs } -> std::convertible_to<Result>;
	};

	// Concept: AddAssignable
	// Describes types Lhs and Rhs where Lhs can be assigned the result of Lhs += Rhs.
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs += rhs must be a reference to Lhs (Lhs&).
	template <typename Lhs, typename Rhs = Lhs>
	concept AddAssignable = requires(Lhs& lhs, Rhs rhs) {
		{ lhs += rhs } -> std::same_as<Lhs&>;
	};

	// Concept: SubtractAssignable
	// Describes types Lhs and Rhs where Lhs can be assigned the result of Lhs -= Rhs.
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs -= rhs must be a reference to Lhs (Lhs&).
	template <typename Lhs, typename Rhs = Lhs>
	concept SubtractAssignable = requires(Lhs& lhs, Rhs rhs) {
		{ lhs -= rhs } -> std::same_as<Lhs&>;
	};

	// Concept: MultiplyAssignable
	// Describes types Lhs and Rhs where Lhs can be assigned the result of Lhs *= Rhs.
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs *= rhs must be a reference to Lhs (Lhs&).
	template <typename Lhs, typename Rhs = Lhs>
	concept MultiplyAssignable = requires(Lhs& lhs, Rhs rhs) {
		{ lhs *= rhs } -> std::same_as<Lhs&>;
	};

	// Concept: DivideAssignable
	// Describes types Lhs and Rhs where Lhs can be assigned the result of Lhs /= Rhs.
	// If only Lhs is provided, Rhs defaults to Lhs.
	// The result type of lhs /= rhs must be a reference to Lhs (Lhs&).
	// Note: Similar to Divisible, this only checks for syntactic validity.
	template <typename Lhs, typename Rhs = Lhs>
	concept DivideAssignable = requires(Lhs& lhs, Rhs rhs) {
		{ lhs /= rhs } -> std::same_as<Lhs&>;
	};
}