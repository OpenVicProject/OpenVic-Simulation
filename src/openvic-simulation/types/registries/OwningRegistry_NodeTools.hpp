#pragma once

#include <iterator>

#include "OwningRegistry.hpp"
#include "OwningRegistry_Search.hpp"

#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/dataloader/IdentifierPolicy.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	inline constexpr NodeTools::KeyValueCallback auto key_value_invalid_callback(std::string_view name) {
		return [name](std::string_view key, ovdl::v2script::ast::Node const*) -> bool {
			spdlog::error_s("Invalid {}: {}", name, key);
			return false;
		};
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr Callback<std::string_view> auto expect_index_by_identifier(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<const SizeType> auto callback,
		IdentifierValidation validation
	) {
		assert_is_locked(registry);
		return [&registry, callback, validation](std::string_view identifier) -> bool {
			if (identifier.empty()) {
				if (validation.allow_empty) {
					return true;
				} else {
					spdlog::log_s(
						validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
						"Invalid {} identifier: empty!", OpenVic::type_name<ValueType>()
					);
					return validation.warn_on_invalid;
				}
			}
			const auto it = find(registry, identifier);
			if (it == registry.end()) {
				spdlog::log_s(
					validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
					"Invalid {} identifier: {}", OpenVic::type_name<ValueType>(), identifier
				);
				return validation.warn_on_invalid;
			}

			return callback(SizeType(std::distance(registry.begin(), it)));
		};
	}
	template<
		IdentifierSyntax Policy = IdentifierSyntax::Either,
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline NodeTools::node_callback_t expect_index(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<const SizeType> auto callback,
		IdentifierValidation validation = {}
	) {
		if constexpr (Policy == IdentifierSyntax::Either) {
			return NodeTools::expect_identifier_or_string(expect_index_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::Identifier) {
			return NodeTools::expect_identifier(expect_index_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::String) {
			return NodeTools::expect_string(expect_index_by_identifier(registry, callback, validation));
		}
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr Callback<std::string_view> auto expect_mutable_item_by_identifier(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		Callback<ValueType&> auto callback,
		IdentifierValidation validation
	) {
		assert_is_locked(registry);
		return [&registry, callback, validation](std::string_view identifier) -> bool {
			if (identifier.empty()) {
				if (validation.allow_empty) {
					return true;
				} else {
					spdlog::log_s(
						validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
						"Invalid {} identifier: empty!", OpenVic::type_name<ValueType>()
					);
					return validation.warn_on_invalid;
				}
			}
			const auto it = find(registry, identifier);
			if (it == registry.end()) {
				spdlog::log_s(
					validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
					"Invalid {} identifier: {}", OpenVic::type_name<ValueType>(), identifier
				);
				return validation.warn_on_invalid;
			}

			return callback(*it);
		};
	}
	template<
		IdentifierSyntax Policy = IdentifierSyntax::Either,
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline NodeTools::node_callback_t expect_mutable_item(
		OwningRegistry<ValueType, SizeType, Allocator>& registry,
		Callback<ValueType&> auto callback,
		IdentifierValidation validation = {}
	) {
		if constexpr (Policy == IdentifierSyntax::Either) {
			return NodeTools::expect_identifier_or_string(expect_mutable_item_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::Identifier) {
			return NodeTools::expect_identifier(expect_mutable_item_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::String) {
			return NodeTools::expect_string(expect_mutable_item_by_identifier(registry, callback, validation));
		}
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline constexpr Callback<std::string_view> auto expect_item_by_identifier(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<ValueType const&> auto callback,
		IdentifierValidation validation
	) {
		assert_is_locked(registry);
		return [&registry, callback, validation](std::string_view identifier) -> bool {
			if (identifier.empty()) {
				if (validation.allow_empty) {
					return true;
				} else {
					spdlog::log_s(
						validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
						"Invalid {} identifier: empty!", OpenVic::type_name<ValueType>()
					);
					return validation.warn_on_invalid;
				}
			}
			const auto it = find(registry, identifier);
			if (it == registry.end()) {
				spdlog::log_s(
					validation.warn_on_invalid ? spdlog::level::warn : spdlog::level::err,
					"Invalid {} identifier: {}", OpenVic::type_name<ValueType>(), identifier
				);
				return validation.warn_on_invalid;
			}

			return callback(*it);
		};
	}
	template<
		IdentifierSyntax Policy = IdentifierSyntax::Either,
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	inline NodeTools::node_callback_t expect_item(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<ValueType const&> auto callback,
		IdentifierValidation validation = {}
	) {
		if constexpr (Policy == IdentifierSyntax::Either) {
			return NodeTools::expect_identifier_or_string(expect_item_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::Identifier) {
			return NodeTools::expect_identifier(expect_item_by_identifier(registry, callback, validation));
		} 
		else if constexpr (Policy == IdentifierSyntax::String) {
			return NodeTools::expect_string(expect_item_by_identifier(registry, callback, validation));
		}
	}
	
	template<
		typename ValueType,
		typename SizeType,
		typename Allocator
	>
	constexpr NodeTools::NodeCallback auto expect_item_dictionary(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<const SizeType, ovdl::v2script::ast::Node const*> auto callback
	) {
		assert_is_locked(registry);
		return NodeTools::expect_list_and_length(
			NodeTools::default_length_callback,
			NodeTools::expect_assign(
				[&registry, callback](
					std::string_view identifier,
					ovdl::v2script::ast::Node const* value
				) -> bool {
					const auto it = find(registry, identifier);
					if (it == registry.end()) {
						return key_value_invalid_callback(OpenVic::type_name<ValueType>())(identifier, value);
					}

					return callback(
						SizeType(std::distance(registry.begin(), it)),
						value
					);
				}
			)
		);
	}

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator,
		FunctorConvertible<fixed_point_t, fixed_point_t> FixedPointFunctor = std::identity
	>
	inline constexpr NodeTools::NodeCallback auto expect_item_decimal_map(
		OwningRegistry<ValueType, SizeType, Allocator> const& registry,
		Callback<fixed_point_map_t<SizeType>&&> auto callback,
		FixedPointFunctor fixed_point_functor = {}
	) {
		assert_is_locked(registry);
		return [&registry, callback, fixed_point_functor](ovdl::v2script::ast::Node const* node) -> bool {
			fixed_point_map_t<SizeType> map;

			bool ret = expect_item_dictionary(
				registry,
				[&map, fixed_point_functor](const SizeType index, ovdl::v2script::ast::Node const* value) -> bool {
					return NodeTools::expect_fixed_point(
						[&map, fixed_point_functor, index](fixed_point_t val) -> bool {
							map.emplace(index, fixed_point_functor(val));
							return true;
						}
					)(value);
				}
			)(node);

			ret &= callback(std::move(map));

			return ret;
		};
	}
}