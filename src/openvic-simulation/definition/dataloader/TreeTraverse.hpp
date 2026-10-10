#pragma once

#include <concepts>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <dryad/node.hpp>

#include <fmt/ranges.h>

#include <spdlog/common.h>

#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/definition/dataloader/ListParser.hpp"
#include "openvic-simulation/definition/dataloader/MapInserter.hpp"
#include "openvic-simulation/definition/dataloader/TraverseResult.hpp"
#include "openvic-simulation/definition/dataloader/Utility.hpp"
#include "openvic-simulation/definition/dataloader/ValueParser.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

namespace OpenVic {
	struct TreeOptions {
		dataloader::DiagnosticLevel unknown_level = dataloader::DiagnosticLevel::NONE;
	};

	template<TreeOptions Options, typename Apply, typename DefaultElement, typename... Elements>
	class TreeTraverse {
	public:
		using parser_type = ovdl::v2script::Parser;
		using parser_pointer_type = parser_type const*;

		using node_type = ovdl::v2script::ast::Node;
		using node_pointer_type = node_type const*;

		using value_node_type = ovdl::v2script::ast::Value;
		using value_node_pointer_type = value_node_type const*;

		using statement_range_iterator = node_type::_children_range<const node_type>::iterator;
		using statement_range_type = dryad::node_range<statement_range_iterator, ovdl::v2script::ast::Statement>;

		static constexpr bool is_apply_empty = std::same_as<Apply, void>;
		using apply_type = std::conditional_t<is_apply_empty, dataloader::empty_type, Apply>;

		static constexpr bool is_default_element_empty = std::same_as<DefaultElement, void>;
		using default_element_target_type = dataloader::conditional_target_type<DefaultElement>;
		using default_element_target_pointer_type = dataloader::conditional_target_pointer_type<DefaultElement>;

		constexpr TreeTraverse()
		requires(is_apply_empty && is_default_element_empty)
		= default;

		constexpr TreeTraverse(apply_type a, default_element_target_pointer_type map, typename Elements::target_type*... refs) :
		    apply_function(std::move(a)), map { map }, targets { refs... } {}

		template<TreeOptions NewOptions>
		constexpr auto options() const {
			return _make_new_traverse_options<NewOptions>(std::make_index_sequence<sizeof...(Elements)> {});
		}

		template<string_literal Key, typename T>
		requires requires { sizeof(dataloader::ValueExtractor<T>); }
		constexpr auto expect_once(T& ref) const {
			return append_key_rule<Key, dataloader::expect_once_key_rule>(ref);
		}

		template<string_literal Key, auto Function, typename T>
		requires requires(parser_pointer_type p, value_node_pointer_type vn, T& o) {
			{ Function({ p, vn, o }) } -> std::convertible_to<Error>;
		} || requires(parser_pointer_type p, value_node_pointer_type vn) {
			typename T::value_type;
			sizeof(dataloader::MapCallback<T>);
			{ Function({ p, vn, std::declval<type_safe::output_parameter<T>>() }) } -> std::convertible_to<Error>;
		}
		constexpr auto expect_many(T& ref) const {
			return append_key_rule<dataloader::expect_many_key_rule<Key, T, Function>>(ref);
		}

		template<string_literal Key, typename T>
		requires requires { sizeof(dataloader::ValueExtractor<T>); }
		constexpr auto expect_many(T& ref) const {
			return expect_many<Key, dataloader::ValueExtractor<T>::extract>(ref);
		}

		template<string_literal Key, typename T>
		requires requires { sizeof(dataloader::ValueExtractor<T>); }
		constexpr auto try_once(T& ref) const {
			return append_key_rule<Key, dataloader::try_once_key_rule>(ref);
		}

		template<string_literal Key, auto Function, typename T>
		requires requires(parser_pointer_type p, value_node_pointer_type vn, T& o) {
			{ Function({ p, vn, o }) } -> std::convertible_to<Error>;
		} || requires(parser_pointer_type p, value_node_pointer_type vn) {
			typename T::value_type;
			sizeof(dataloader::MapCallback<T>);
			{ Function({ p, vn, std::declval<type_safe::output_parameter<T>>() }) } -> std::convertible_to<Error>;
		}
		constexpr auto try_many(T& ref) const {
			return append_key_rule<dataloader::try_many_key_rule<Key, T, Function>>(ref);
		}

		template<string_literal Key, typename T>
		requires requires { sizeof(dataloader::ValueExtractor<T>); }
		constexpr auto try_many(T& ref) const {
			return try_many<Key, dataloader::ValueExtractor<T>::extract>(ref);
		}

		template<string_literal Key, typename T>
		requires requires { sizeof(dataloader::ValueExtractor<T>); }
		constexpr auto set_when(T& ref) const {
			return append_key_rule<Key, dataloader::set_when_key_rule>(ref);
		}

		template<dataloader::DefaultOptions DefOptions = {}, typename MapT>
		requires requires { typename MapT::value_type; }
		constexpr auto try_default(MapT& map) const {
			return try_default<typename MapT::value_type, DefOptions>(map);
		}

		template<typename ValueT, dataloader::DefaultOptions DefOptions = {}, typename MapT>
		requires requires {
			sizeof(dataloader::ValueExtractor<type_safe::output_parameter<ValueT>>);
			sizeof(dataloader::MapCallback<MapT>);
		}
		constexpr auto try_default(MapT& map) const {
			return set_default<dataloader::try_default_map_rule<DefOptions, MapT, ValueT>>(map);
		}

		template<template<typename, typename> typename DefaultRuleTemplate, typename MapT>
		requires requires { typename MapT::value_type; }
		constexpr auto set_default_rule(MapT& map) const {
			return set_default_rule<typename MapT::value_type, DefaultRuleTemplate<MapT, typename MapT::value_type>>(map);
		}

		template<typename ValueT, template<typename, typename> typename DefaultRuleTemplate, typename MapT>
		requires requires {
			typename DefaultRuleTemplate<MapT, ValueT>::value_type;
		} && std::same_as<ValueT, typename DefaultRuleTemplate<MapT, ValueT>::value_type>
		constexpr auto set_default_rule(MapT& map) const {
			return set_default_rule<DefaultRuleTemplate<MapT, ValueT>>(map);
		}

		template<typename DefaultRule, typename MapT>
		requires requires(dataloader::DefaultFunctionArguments<MapT> args) {
			typename DefaultRule::target_type;
			typename DefaultRule::value_type;
			{ DefaultRule::call(args) } -> std::same_as<Error>;
		} && std::same_as<MapT, typename DefaultRule::target_type>
		constexpr auto set_default_rule(MapT& map) const {
			return _make_new_traverse_default<DefaultRule, typename DefaultRule::value_type>(
			    map, std::make_index_sequence<sizeof...(Elements)> {}
			);
		}

		template<strict_regular_invocable_r<Error, dataloader::ApplyFunctionArguments> Func>
		constexpr auto apply(Func&& func) const {
			using NewApply = std::decay_t<Func>;
			return _make_new_traverse_apply<NewApply>(
			    std::forward<Func>(func), std::make_index_sequence<sizeof...(Elements)> {}
			);
		}

		template<dataloader::key_rule_concept NewElement, std::same_as<typename NewElement::target_type> T>
		constexpr auto append_key_rule(T& ref) const {
			return _make_new_traverse_append<NewElement>(std::make_index_sequence<sizeof...(Elements)> {}, ref);
		}

		template<string_literal Key, template<string_literal, typename> typename NewElement, typename T>
		requires dataloader::key_rule_template_fits<NewElement, Key, T>
		constexpr auto append_key_rule(T& ref) const {
			return _make_new_traverse_append<NewElement<Key, T>>(std::make_index_sequence<sizeof...(Elements)> {}, ref);
		}

		TraverseResult operator()(ovdl::v2script::Parser const& parser, node_pointer_type node) const {
			return operator()(&parser, node);
		}

		TraverseResult operator()(ovdl::v2script::Parser const& parser) const {
			TraverseResult result { { &parser }, Error::OK };
			operator()(result, parser.get_file_node(), parser.get_file_node()->statements());
			return result;
		}

		TraverseResult operator()(node_pointer_type node) const {
			return operator()(nullptr, node);
		}

		TraverseResult operator()(parser_pointer_type parser, node_pointer_type node) const {
			TraverseResult result { { parser }, Error::OK };
			operator()(result, node);
			return result;
		}

		TraverseResult& operator()(TraverseResult& result, node_pointer_type node) const {
			using namespace ovdl::v2script::ast;

			auto statements = [&]() -> statement_range_type {
				if (auto* ft = dryad::node_try_cast<FileTree>(node)) {
					return ft->statements();
				}

				if (auto* lv = dryad::node_try_cast<ListValue>(node)) {
					return lv->statements();
				}

				result.error = Error::FAILED;
				result.diagnostics.error(node).with_message(
				    "Expected a file tree or list value, found {}", get_kind_name(node->kind())
				);
				return dryad::make_node_range<Statement>(
				    statement_range_iterator::from_ptr(nullptr), statement_range_iterator::from_ptr(nullptr)
				);
			}();

			if (result.error != Error::OK) {
				return result;
			}

			return operator()(result, node, statements);
		}

		TraverseResult& operator()(TraverseResult& result, node_pointer_type node, statement_range_type statements) const {
			using namespace ovdl::v2script::ast;

			std::array<bool, sizeof...(Elements)> found {};

			auto key_list = ([&]()->std::array<ovdl::symbol<>, sizeof...(Elements)> {
				if (result.diagnostics.parser() == nullptr) {
					return {};
				}

				return { result.diagnostics.parser()->find_intern(Elements::key)... };
			}());

			result.error = initialize_all(node, result, std::make_index_sequence<sizeof...(Elements)> {});

			for (Statement const* s : statements) {
				if (result.diagnostics.has_fatal_error()) {
					break;
				}

				auto* as = dryad::node_try_cast<AssignStatement>(s);
				if (!as) {
					continue;
				}

				auto* left = dryad::node_try_cast<FlatValue>(as->left());
				if (!left) {
					continue;
				}

				bool handled = false;

				// Linear search – sizeof...(Elements) is tiny, so this is fine
				// and still completely optimisable by the compiler.
				const ovdl::symbol<> symbol = left->value();
				handled = try_all(
				    { result, symbol, as->right(), found }, key_list, std::make_index_sequence<sizeof...(Elements)> {}
				);

				if (handled) {
					if (result.error != Error::OK && result.error != Error::SKIP) {
						return result;
					}
					handled = result.error == Error::OK;
				} else if constexpr (!is_default_element_empty) {
					Error err = Error::OK;
					if (err = DefaultElement::call({ result, as, map }); err != Error::OK && err != Error::SKIP) {
						return result;
					}
					handled = err == Error::OK;
				}

				if (handled) {
					continue;
				}

				if constexpr (!is_apply_empty) {
					if (Error err = apply_function(dataloader::ApplyFunctionArguments { result, as });
					    err != Error::OK && err != Error::SKIP) {
						return result;
					}
					handled = result.error == Error::OK;
				}

				if constexpr (Options.unknown_level != dataloader::DiagnosticLevel::NONE) {
					if (!handled) {
						if constexpr (Options.unknown_level <= dataloader::DiagnosticLevel::ERROR) {
							result.error = Error::FAILED;
						}
						result.diagnostics.report(Options.unknown_level, s).with_message("Unknown key: {}", symbol.view());
					}
				}
			}

			result.error = finalize_all(result, node, found, std::make_index_sequence<sizeof...(Elements)> {});
			return result;
		}

	private:
		std::tuple<typename Elements::target_type*...> targets;
		OV_NO_UNIQUE_ADDRESS apply_type apply_function;
		OV_NO_UNIQUE_ADDRESS default_element_target_pointer_type map;

		struct TryArguments {
			TraverseResult& traverse;
			ovdl::symbol<> key;
			value_node_pointer_type value = nullptr;
			std::array<bool, sizeof...(Elements)>& found_list;
		};

		template<typename NewElement, std::size_t... Is, typename T>
		constexpr auto _make_new_traverse_append(std::index_sequence<Is...>, T& ref) const {
			using NewTraverse = TreeTraverse<Options, Apply, DefaultElement, Elements..., NewElement>;
			return NewTraverse {
				apply_function,
				map,
				std::get<Is>(targets)...,
				&ref,
			};
		}

		template<TreeOptions NewOptions, std::size_t... Is>
		constexpr auto _make_new_traverse_options(std::index_sequence<Is...>) const {
			using NewTraverse = TreeTraverse<NewOptions, Apply, DefaultElement, Elements...>;
			return NewTraverse { apply_function, map, *std::get<Is>(targets)... };
		}

		template<typename NewApply, std::size_t... Is>
		requires(is_apply_empty && !std::same_as<NewApply, void>)
		constexpr auto _make_new_traverse_apply(NewApply&& func, std::index_sequence<Is...>) const {
			using NewTraverse = TreeTraverse<Options, NewApply, DefaultElement, Elements...>;
			return NewTraverse {
				std::move(func),
				map,
				*std::get<Is>(targets)...,
			};
		}

		template<typename NewDefaultElement, std::size_t... Is, typename MapT>
		requires(is_default_element_empty && !std::same_as<NewDefaultElement, void>)
		constexpr auto _make_new_traverse_default(MapT& ref, std::index_sequence<Is...>) const {
			using NewTraverse = TreeTraverse<Options, Apply, NewDefaultElement, Elements...>;
			return NewTraverse {
				apply_function,
				&ref,
				*std::get<Is>(targets)...,
			};
		}

		template<std::size_t... Is>
		Error initialize_all(node_pointer_type root_node, TraverseResult& traverse, std::index_sequence<Is...>) const {
			bool failed = false;
			((failed |=
			  Elements::initialize(dataloader::TraverseInitializeArguments { traverse, root_node, *std::get<Is>(targets) }) !=
			  Error::OK),
			 ...);

			return failed ? Error::FAILED : Error::OK;
		}

		template<std::size_t... Is>
		bool try_all(
		    TryArguments args, std::array<ovdl::symbol<>, sizeof...(Elements)> const& key_list, std::index_sequence<Is...>
		) const {
			bool handled = false;

			if (args.traverse.diagnostics.parser() == nullptr) {
				// Iterates Elements until Elements::key == key
				((handled = handled ||
				            (Elements::key == args.key.view() &&
				             Elements::try_extract(
				                 dataloader::TraverseExtractArguments {
				                     args.traverse, args.value, *std::get<Is>(targets), args.found_list[Is] }
				             ))),
				 ...);

				return handled;
			}

			for (size_t i = 0; i < key_list.size(); i++) {
				const ovdl::symbol<> symbol = key_list[i];
				if (!symbol || symbol != args.key) {
					continue;
				}

				// Iterates Elements until Is == i
				((handled =
				      handled ||
				      (Is == i &&
				       Elements::try_extract(
				           dataloader::TraverseExtractArguments {
				               args.traverse,
				               args.value,
				               *std::get<Is>(targets),
				               args.found_list[Is],
				           }
				       ))),
				 ...);

				if (handled) {
					return true;
				}
			}

			return false;
		}

		template<std::size_t... Is>
		Error finalize_all(
		    TraverseResult& traverse,
		    node_pointer_type node,
		    std::array<bool, sizeof...(Elements)> const& found,
		    std::index_sequence<Is...>
		) const {
			// TODO: make inplace_vector
			memory::vector<std::string_view> expected_keys;
			expected_keys.reserve(sizeof...(Elements));

			bool failed = false;
			((failed |=
			  Elements::finalize(dataloader::TraverseFinalizeArguments { traverse, node, expected_keys, found[Is] }) !=
			  Error::OK),
			 ...);

			if constexpr (!is_default_element_empty) {
				failed |=
				    DefaultElement::finalize(dataloader::TraverseFinalizeArguments { traverse, node, expected_keys, true }) !=
				    Error::OK;
			}

			if (!expected_keys.empty() && failed) {
				if (expected_keys.size() == 1) {
					traverse.diagnostics.error(node).with_message("Expected key: {}", expected_keys[0]);
				} else {
					traverse.diagnostics.error(node).with_message("Expected keys: [{}]", fmt::join(expected_keys, ", "));
				}
				return Error::FAILED;
			}

			return failed ? Error::FAILED : Error::OK;
		}
	};

	inline constexpr TreeTraverse<{}, void, void> Traverse {};
}
