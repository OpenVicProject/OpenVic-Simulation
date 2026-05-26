#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <string_view>

#include "OwningRegistry.hpp"

#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr void assert_is_locked(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry
	) {
		assert(registry.is_locked());
		if (!registry.is_locked()) {
			spdlog::error_s("registry {} is used but not yet locked!", OpenVic::type_name<ValueType>());
		}
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires is_strongly_typed<SizeType>
	[[nodiscard]] inline constexpr TypedSpan<SizeType, ValueType> get_values(OwningRegistry<ValueType, SizeType, Allocator>& registry) {
		assert_is_locked(registry);
		return { registry };
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires is_strongly_typed<SizeType>
	[[nodiscard]] inline constexpr TypedSpan<SizeType, const ValueType> get_values(OwningRegistry<ValueType, SizeType, Allocator> const& registry) {
		assert_is_locked(registry);
		return { registry };
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires (!is_strongly_typed<SizeType>)
	[[nodiscard]] inline constexpr std::span<ValueType> get_values(OwningRegistry<ValueType, SizeType, Allocator>& registry) {
		assert_is_locked(registry);
		return { registry.data(), registry.size() };
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires (!is_strongly_typed<SizeType>)
	[[nodiscard]] inline constexpr std::span<const ValueType> get_values(OwningRegistry<ValueType, SizeType, Allocator> const& registry) {
		assert_is_locked(registry);
		return { registry.data(), registry.size() };
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires has_get_identifier<ValueType>
	[[nodiscard]] inline constexpr auto get_identifiers(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry
	) {
		assert_is_locked(registry);
		return registry | std::views::transform([](auto const& x) -> std::string_view {
			return x.get_identifier();
		});
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires has_get_identifier<ValueType>
	[[nodiscard]] inline constexpr bool has_identifier(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		std::string_view identifier
	) {
		return std::ranges::any_of(registry.begin(), registry.end(), [&](ValueType const& value) {
			return value.get_identifier() == identifier;
		});
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires has_get_identifier<ValueType>
	[[nodiscard]] inline constexpr OwningRegistry<ValueType, SizeType, Allocator>::iterator find(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		std::string_view identifier
	) {
		assert_is_locked(registry);
		return std::find_if(registry.begin(), registry.end(), [&](ValueType const& value) {
			return value.get_identifier() == identifier;
		});
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	> requires has_get_identifier<ValueType>
	[[nodiscard]] inline constexpr OwningRegistry<ValueType, SizeType, Allocator>::const_iterator find(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		std::string_view identifier
	) {
		assert_is_locked(registry);
		return std::find_if(registry.begin(), registry.end(), [&](ValueType const& value) {
			return value.get_identifier() == identifier;
		});
	}
}