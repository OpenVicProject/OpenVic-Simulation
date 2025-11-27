#pragma once

#include <concepts>
#include <cstdint>
#include <random>
#include <type_traits>
#include <utility>

#include "openvic-simulation/core/Typedefs.hpp"

#include <XoshiroCpp.hpp>

namespace OpenVic {
	template<std::uniform_random_bit_generator T>
	struct RandomGenerator {
		using generator_type = T;

		using state_type = std::conditional_t<
			requires { typename generator_type::state_type; },
			typename generator_type::state_type,
			void
		>;
		using result_type = std::conditional_t<
			requires { typename generator_type::result_type; },
			typename generator_type::result_type,
			void
		>;

		[[nodiscard]] OV_ALWAYS_INLINE explicit constexpr RandomGenerator()
		requires requires {
			{ T {} };
		}
			: _generator {} {}

		[[nodiscard]] OV_ALWAYS_INLINE explicit constexpr RandomGenerator(std::uint64_t seed)
		requires requires {
			{ T { seed } };
		}
			: _generator { seed } {}

		[[nodiscard]] OV_ALWAYS_INLINE explicit constexpr RandomGenerator(state_type state)
		requires requires {
			{ T { state } };
		}
			: _generator { state } {}

		OV_ALWAYS_INLINE constexpr result_type operator()() {
			return _generator();
		}

		[[nodiscard]] OV_ALWAYS_INLINE static constexpr result_type min() {
			return T::min();
		}

		[[nodiscard]] OV_ALWAYS_INLINE static constexpr result_type max() {
			return T::max();
		}

		[[nodiscard]] OV_ALWAYS_INLINE friend bool operator==(RandomGenerator const& lhs, RandomGenerator const& rhs)
		requires requires {
			{ std::declval<T>() == std::declval<T>() } -> std::same_as<bool>;
		}
		{
			return lhs._generator == rhs._generator;
		}

		[[nodiscard]] OV_ALWAYS_INLINE friend bool operator!=(RandomGenerator const& lhs, RandomGenerator const& rhs)
		requires requires {
			{ std::declval<T>() != std::declval<T>() } -> std::same_as<bool>;
		}
		{
			return lhs._generator != rhs._generator;
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr T& generator() {
			return _generator;
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr T const& generator() const {
			return _generator;
		}

	private:
		T _generator;
	};

	struct RandomU32 : RandomGenerator<XoshiroCpp::Xoshiro128StarStar> { using RandomGenerator::RandomGenerator; };
	struct RandomU64 : RandomGenerator<XoshiroCpp::Xoshiro256StarStar> { using RandomGenerator::RandomGenerator; };
}
