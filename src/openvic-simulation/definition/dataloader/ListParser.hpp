#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <spdlog/common.h>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/definition/dataloader/MapInserter.hpp"
#include "openvic-simulation/definition/dataloader/TraverseResult.hpp"
#include "openvic-simulation/definition/dataloader/ValueParser.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

namespace OpenVic::dataloader {
	template<typename T>
	concept key_rule_concept = requires {
		typename T::target_type;
		{ T::key } -> string_literal_concept;
	};

	template<typename Rule, string_literal Key, typename T>
	concept key_rule_fits = key_rule_concept<Rule> && std::same_as<T, typename Rule::target_type> && (Key == Rule::key);

	template<template<string_literal, typename> typename Rule, string_literal Key, typename T>
	concept key_rule_template_fits = key_rule_fits<Rule<Key, T>, Key, T>;

	template<typename T>
	struct TraverseInitializeArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::Node const* root_node;
		T& out;
	};

	template<typename T>
	struct TraverseExtractArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::Value const* node;
		T& out;
		bool& was_found;
	};

	struct TraverseFinalizeArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::Node const* node;
		memory::vector<std::string_view>& expected;
		bool was_found;
	};

	template<typename T>
	struct traverse_rule {
		using target_type = T;

		static Error initialize(TraverseInitializeArguments<T> args) {
			if constexpr (requires {
				              { ValueExtractor<T>::initialize({ args.parser, args.out }) } -> std::same_as<Error>;
			              }) {
				return ValueExtractor<T>::initialize({ args.parser, args.out });
			}
			return Error::OK;
		}
	};

	template<typename T>
	struct try_rule {
		static Error finalize(TraverseFinalizeArguments args) {
			if constexpr (requires {
				              { ValueExtractor<T>::finalize({ args.traverse }) } -> std::same_as<Error>;
			              }) {
				return ValueExtractor<T>::finalize({ args.traverse });
			}
			return Error::OK;
		}
	};

	template<string_literal Key, typename T>
	struct traverse_key_rule : traverse_rule<T> {
		static constexpr string_literal key = Key;
	};

	template<string_literal Key, typename T>
	struct expect_key_rule : traverse_key_rule<Key, T> {
		static Error finalize(TraverseFinalizeArguments args) {
			if (!args.was_found) {
				args.expected.emplace_back(Key);
				return Error::FAILED;
			}
			return Error::OK;
		}
	};

	template<string_literal Key, typename T>
	struct try_key_rule : traverse_key_rule<Key, T>, try_rule<T> {};

	template<string_literal Key, typename T>
	struct expect_once_key_rule : expect_key_rule<Key, T> {
		static bool try_extract(TraverseExtractArguments<T> args) {
			args.traverse.error = Error::FAILED;
			if (args.was_found) {
				args.traverse.diagnostics.error(args.node).with_message("Found multiple {} keys", Key);
				return true;
			}

			args.traverse.error = ValueExtractor<T>::extract({ args.traverse, args.node, args.out });
			if (args.traverse.error == Error::OK) {
				args.was_found = true;
			}
			return true;
		}
	};

	template<
	    string_literal Key,
	    typename T,
	    auto Function,
	    typename ItemType = std::conditional_t<requires { typename T::value_type; }, typename T::value_type, T>,
	    typename ParamType = std::conditional_t<std::same_as<ItemType, T>, T, type_safe::output_parameter<ItemType>>>
	requires strict_regular_invocable_r<std::decay_t<decltype(Function)>, Error, ValueExtractorArguments<ParamType>>
	struct expect_many_key_rule : expect_key_rule<Key, T> {
		using item_type = ItemType;
		using parameter_type = ParamType;

		static bool try_extract(TraverseExtractArguments<T> args) {
			if constexpr (std::same_as<item_type, T>) {
				args.traverse.error = Function({ args.traverse, args.node, args.out });
			} else {
				type_safe::deferred_construction<item_type> item;
				args.traverse.error = Function({ args.traverse, args.node, type_safe::out(item) });
				if (args.traverse.error == Error::OK && item.has_value()) {
					MapCallback<T>::insert({ args.traverse, args.node, args.out }, item.value());
				}
			}
			return true;
		}
	};

	template<string_literal Key, typename T>
	struct try_once_key_rule : try_key_rule<Key, T> {
		static bool try_extract(TraverseExtractArguments<T> args) {
			args.traverse.error = Error::FAILED;
			if (args.was_found) {
				args.traverse.diagnostics.error(args.node).with_message("Found multiple {} keys", Key);
				return true;
			}

			args.traverse.error = ValueExtractor<T>::extract({ args.traverse, args.node, args.out });
			if (args.traverse.error == Error::OK) {
				args.was_found = true;
			}
			return true;
		}
	};

	template<
	    string_literal Key,
	    typename T,
	    auto Function,
	    typename ItemType = std::conditional_t<requires { typename T::value_type; }, typename T::value_type, T>,
	    typename ParamType = std::conditional_t<std::same_as<ItemType, T>, T, type_safe::output_parameter<ItemType>>>
	requires strict_regular_invocable_r<std::decay_t<decltype(Function)>, Error, ValueExtractorArguments<ParamType>>
	struct try_many_key_rule : try_key_rule<Key, T> {
		using item_type = ItemType;
		using parameter_type = ParamType;

		static bool try_extract(TraverseExtractArguments<T> args) {
			if constexpr (std::same_as<item_type, T>) {
				args.traverse.error = Function({ args.traverse, args.node, args.out });
			} else {
				type_safe::deferred_construction<item_type> item;
				args.traverse.error = Function({ args.traverse, args.node, type_safe::out(item) });
				if (args.traverse.error == Error::OK && item.has_value()) {
					MapCallback<T>::insert({ args.traverse, args.node, args.out }, item.value());
				}
			}
			return true;
		}
	};

	template<string_literal Key, typename T>
	struct set_when_key_rule : try_key_rule<Key, T> {
		static bool try_extract(TraverseExtractArguments<T> args) {
			args.traverse.error = ValueExtractor<T>::try_extract({ args.traverse, args.node, args.out });
			return true;
		}
	};

	struct DefaultOptions {
		DiagnosticLevel duplicates_level = DiagnosticLevel::WARN;
	};

	template<typename MapT>
	struct DefaultFunctionArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::AssignStatement const* node;
		MapT& map;
	};

	template<DefaultOptions Options, typename MapT, typename ValueT = typename MapT::value_type>
	struct try_default_map_rule : try_rule<type_safe::output_parameter<ValueT>> {
		using target_type = MapT;
		using value_type = ValueT;

		static Error call(DefaultFunctionArguments<MapT> args) {
			auto* left = dryad::node_try_cast<ovdl::v2script::ast::FlatValue>(args.node->left());
			if (!left) {
				return Error::SKIP;
			}

			if constexpr (Options.duplicates_level != DiagnosticLevel::NONE) {
				if (MapCallback<MapT>::has({ args.traverse, left, args.map })) {
					args.traverse.diagnostics.report(Options.duplicates_level, left)
					    .with_message("Found multiple {} keys", left->value().view());
					if constexpr (Options.duplicates_level <= DiagnosticLevel::ERROR) {
						return Error::FAILED;
					}
				}
			}

			type_safe::deferred_construction<value_type> value;
			Error err = ValueExtractor<type_safe::output_parameter<value_type>>::extract(
			    { args.traverse, left, type_safe::out(value) }
			);

			if (err == Error::OK && value.has_value()) {
				return MapCallback<MapT>::insert({ args.traverse, left, args.map }, value.value());
			}
			return err;
		}
	};

	struct ApplyFunctionArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::AssignStatement const* node;
	};
}
