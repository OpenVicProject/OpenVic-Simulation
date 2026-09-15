#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <type_safe/optional_ref.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Diagnostics.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ListOptions.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ValueParser.hpp"

namespace OpenVic::dataloader::dsl {
	template<typename... Rules>
	requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
	constexpr auto list(Rules&&... rules);

	template<typename T>
	struct _ref_setter;

	template<typename T>
	struct _ref_sink;

	template<typename T>
	struct _ref {
		T& reference;

		template<typename Func, typename... Rules>
		requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
		constexpr auto try_reservable_for(Func&& func, Rules&&... rules) {
			static_assert(
			    requires { list(OV_FWD(rules)...).try_reserve_for(reference).sink(OV_FWD(func)); },
			    "Requires #include \"openvic-simulation/definition/dataloader/dsl/List.hpp\""
			);

			return list(OV_FWD(rules)...).try_reserve_for(reference).sink(OV_FWD(func));
		}

		template<typename... Rules>
		requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
		constexpr auto emplace_back(Rules&&... rules)
		requires requires(T t) {
			typename T::value_type;
			t.emplace_back(std::declval<typename T::value_type&&>());
		}
		{
			return try_reservable_for(
			    [&reference = this->reference]<typename... Args>(
			        TraverseContext&, auto, ast::Node const*, Args&&... args
			    ) mutable
			    requires std::constructible_from<typename T::value_type, Args...>
			    {
				    reference.emplace_back(OV_FWD(args)...);
				    return std::true_type {};
			    },
			    OV_FWD(rules)...
			);
		}

		template<typename... Rules>
		requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
		constexpr auto try_emplace(Rules&&... rules)
		requires requires(T t) {
			typename T::value_type;
			typename T::key_type;
			typename T::mapped_type;
			t.try_emplace(std::declval<typename T::key_type const&>(), std::declval<typename T::mapped_type&&>());
		}
		{
			return try_reservable_for(
			    [&reference = this->reference]<typename Key, typename Node, typename... Args>(
			        TraverseContext& ctx, auto, Node const* node, Key&& key, Args&&... args
			    ) mutable
			    requires(std::convertible_to<Key, typename T::key_type> ||
			             std::constructible_from<typename T::key_type, Key>) &&
			                std::constructible_from<typename T::mapped_type, Args...>
			    {
				    if constexpr (type_with_failable_try_emplace<T, Key, Args...>) {
					    auto result = reference.try_emplace(OV_FWD(key), OV_FWD(args)...);
					    if (!result.second) {
						    diagnostic::found_duplicate_map_key(ctx, node, key);
					    }
					    return result.second;
				    } else {
					    reference.try_emplace(OV_FWD(key), OV_FWD(args)...);
					    return std::true_type {};
				    }
			    },
			    OV_FWD(rules)...
			);
		}

		template<typename... Rules>
		requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
		constexpr auto emplace(Rules&&... rules)
		requires requires(T t) {
			typename T::value_type;
			t.emplace(std::declval<typename T::value_type&&>());
		}
		{
			return try_reservable_for(
			    [&reference = this->reference](TraverseContext&, auto, ast::Node const*, auto&&... args) mutable
			    requires std::constructible_from<typename T::value_type, decltype(args)...>
			    {
				    reference.emplace(OV_FWD(args)...);
				    return std::true_type {};
			    },
			    OV_FWD(rules)...
			);
		}

		template<typename... Rules>
		requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
		constexpr auto construct(Rules&&... rules) {
			auto construct_func = [self = OV_MOV(*this)](TraverseContext&, auto, ast::Node const*, auto&&... args) mutable
			requires requires { T(OV_FWD(args)...); }
			{
				self.reference = T(OV_FWD(args)...);
				return std::true_type {};
			};

			static_assert(
			    requires { list(OV_FWD(rules)...).sink(OV_MOV(construct_func)); },
			    "Requires #include \"openvic-simulation/definition/dataloader/dsl/List.hpp\""
			);

			return list(OV_FWD(rules)...).sink(OV_MOV(construct_func));
		}

		constexpr auto setter() {
			return _ref_setter<T> { reference };
		}

		constexpr auto sink() {
			return _ref_sink<T> { reference };
		}
	};

	template<typename T>
	constexpr auto ref(T& ref) {
		return _ref<T> { ref };
	}

	template<typename T>
	struct _ref_setter {
		static_assert(requires { sizeof(ValueParser<T>); }, "ValueParser<T> must be defined.");
		static constexpr std::bool_constant<sizeof(ValueParser<T>) != 0> stores_value_parser {};

		T& reference;

		auto event_generate_parsers(TraverseContext& ctx)
		requires(stores_value_parser())
		{
			auto parser = ValueParser<T> {};
			if constexpr (requires { parser.make_preparation(ctx); }) {
				parser.make_preparation(ctx);
			}
			return parser;
		}

		template<typename TraverseData, typename Node>
		auto event_process(TraverseContext& ctx, TraverseData& data, Node const* node) {
			if constexpr (node_with_value_symbol<Node>) {
				return data.template parser_for<T>().parse(ctx, data, node, type_safe::opt_ref(reference));
			} else {
				return visit_flat(ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					return event_process(ctx, data, fv);
				});
			}
		}

		template<typename TraverseData, typename Node, typename Sink>
		bool event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) {
			return event_process(ctx, data, node);
		}
	};

	template<typename T>
	struct _ref_sink {
		T const& reference;

		template<typename TraverseData, typename Node, typename Sink>
		bool event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) const
		requires invocable_sink_with<Sink, TraverseData, T const&>
		{
			return sink(ctx, data, node, reference);
		}
	};
}
