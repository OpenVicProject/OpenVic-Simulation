#pragma once

#include <string_view>

#include "OwningRegistry.hpp"
#include "OwningRegistry_Search.hpp"

#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	inline bool duplicate_fail_cb(std::string_view registry_name, std::string_view duplicate_identifier) {
		spdlog::error_s(
			"Failure adding item to the {} registry - an item with the identifier \"{}\" already exists!",
			registry_name, duplicate_identifier
		);
		return false;
	}
	inline bool duplicate_warning_cb(std::string_view registry_name, std::string_view duplicate_identifier) {
		spdlog::warn_s(
			"Warning adding item to the {} registry - an item with the identifier \"{}\" already exists!",
			registry_name, duplicate_identifier
		);
		return true;
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline void lock_registry(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		const bool should_log = false
	) {
		if (registry.is_locked()) {
			spdlog::error_s("Failed to lock {} registry - already locked!", OpenVic::type_name<ValueType>());
		} else {
			registry.is_locked_internal(owning_registry_internal_tag{}) = true;
			if (should_log) {
				SPDLOG_INFO("Locked {} registry after registering {} items", OpenVic::type_name<ValueType>(), registry.size());
			}
		}
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr bool validate_emplacement(OwningRegistry<ValueType, SizeType, Allocator>& registry) {
		assert(!registry.is_locked());
		if (registry.is_locked()) {
			spdlog::error_s("Cannot emplace_item to the {} registry - locked!", OpenVic::type_name<ValueType>());
			return false;
		}

		assert(registry.size() < registry.capacity());
		if (registry.size() >= registry.capacity()) {
			spdlog::error_s("Cannot emplace_item to the {} registry - no capacity!", OpenVic::type_name<ValueType>());
			return false;
		}

		return true;
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator,
		typename... Args
	>
	inline constexpr bool emplace_item(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		Args&&... args
	) {
		if (!validate_emplacement(registry)) {
			return false;
		}

		registry.values_internal(owning_registry_internal_tag{}).emplace_back(std::forward<Args>(args)...);
		return true;
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator,
		typename... Args
	>
	inline constexpr bool try_emplace_item(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		const std::string_view identifier,
		Callback<std::string_view, std::string_view> auto duplicate_callback,
		Args&&... args
	) {
		if (has_identifier(registry, identifier)) {
			return duplicate_callback(OpenVic::type_name<ValueType>(), identifier);
		}

		return emplace_item(registry, std::forward<Args>(args)...);
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator,
		typename... Args
	>
	inline constexpr bool try_emplace_item(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		const std::string_view identifier,
		Args&&... args
	) {
		return try_emplace_item(registry, identifier, duplicate_fail_cb, std::forward<Args>(args)...);
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr bool emplace_via_move(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		ValueType&& item
	) {
		if (!validate_emplacement(registry)) {
			return false;
		}

		registry.values_internal(owning_registry_internal_tag{}).emplace_back(std::move(item));
		return true;
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr bool try_emplace_via_move(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		const std::string_view identifier,
		Callback<std::string_view, std::string_view> auto duplicate_callback,
		ValueType&& item
	) {
		if (has_identifier(registry, identifier)) {
			return duplicate_callback(OpenVic::type_name<ValueType>(), identifier);
		}

		return emplace_via_move(registry, std::move(item));
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr bool try_emplace_via_move(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		const std::string_view identifier,
		ValueType&& item
	) {
		return try_emplace_via_move(registry, identifier, duplicate_fail_cb, std::move(item));
	}
}