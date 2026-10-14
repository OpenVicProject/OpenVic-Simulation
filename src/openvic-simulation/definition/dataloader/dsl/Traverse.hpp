#pragma once

#include <concepts>
#include <type_traits>

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ValueParser.hpp"

namespace OpenVic::dataloader::dsl {
	template<typename TreeRule>
	struct Traverse_t {
		static constexpr auto has_tree_rule = !std::same_as<TreeRule, void>;

		std::conditional_t<has_tree_rule, TreeRule, empty> tree_rule;

		struct TraverseStackData {
			using prepare_result_type =
			    decltype(std::declval<TreeRule>().event_generate_parsers(std::declval<TraverseContext&>()));
			static constexpr std::bool_constant<!std::same_as<prepare_result_type, void>> has_prepare_result_type {};

			OV_NO_UNIQUE_ADDRESS std::conditional_t<has_prepare_result_type, prepare_result_type, empty> parsers;

			template<typename T, typename Tuple>
			struct has_type;

			template<typename T>
			struct has_type<T, empty> : std::false_type {};

			template<typename T>
			struct has_type<T, void> : std::false_type {};

			template<typename T>
			struct empty_test {
				char c;
				OV_NO_UNIQUE_ADDRESS T t;
			};

			template<typename T>
			static constexpr bool can_be_zero_bytes = sizeof(empty_test<T>) == 1;

			template<typename T, typename... Us>
			struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

			template<typename T>
			auto parser_for() {
				if constexpr (has_type<ValueParser<T>, prepare_result_type>::value) {
					return std::get<ValueParser<T>>(parsers);
				} else {
					static_assert(requires { sizeof(ValueParser<T>); }, "ValueParser<T> must be defined.");
					return ValueParser<T> {};
				}
			}

			template<typename T>
			auto parser_for() const {
				if constexpr (has_type<ValueParser<T>, prepare_result_type>::value) {
					return std::get<ValueParser<T>>(parsers);
				} else {
					static_assert(requires { sizeof(ValueParser<T>); }, "ValueParser<T> must be defined.");
					return ValueParser<T> {};
				}
			}
		};

		template<rule NewTreeRule>
		constexpr auto operator()(NewTreeRule&& rule) const
		requires(!has_tree_rule)
		{
			return Traverse_t<NewTreeRule> { OV_FWD(rule) };
		}

		TraverseContext operator()(ovdl::v2script::Parser const& parser, node_type const* node) {
			return operator()(&parser, node);
		}

		TraverseContext operator()(ovdl::v2script::Parser const& parser) {
			return operator()(&parser, parser.get_file_node());
		}

		TraverseContext operator()(node_type const* node) {
			return operator()(nullptr, node);
		}

		TraverseContext operator()(ovdl::v2script::Parser const* parser, node_type const* node) {
			TraverseContext ctx { DiagnosticBag { parser } };
			if constexpr (has_tree_rule) {
				TraverseStackData data = [&] {
					if constexpr (rule_has_generate_parsers<TreeRule> && TraverseStackData::has_prepare_result_type) {
						return TraverseStackData { tree_rule.event_generate_parsers(ctx) };
					} else {
						return TraverseStackData {};
					}
				}();
				if constexpr (rule_has_prepare<TreeRule>) {
					tree_rule.event_prepare(ctx);
				}
				tree_rule.event_process(ctx, data, node);
			}
			return ctx;
		}
	};

	inline constexpr Traverse_t<void> Traverse {};
}
