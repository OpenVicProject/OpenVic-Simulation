#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <type_safe/optional_ref.hpp>
#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/core/object/FixedPoint/String.hpp"
#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Diagnostics.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader {
	namespace ast = ovdl::v2script::ast;

	template<typename T>
	struct ValueParser;

	template<typename T>
	struct ValueParser<type_safe::output_parameter<T>> {
		static_assert(requires { sizeof(ValueParser<T>); }, "ValueParser<T> must be defined.");

		OV_NO_UNIQUE_ADDRESS ValueParser<T> parser;

		void make_preparation(dsl::TraverseContext& ctx)
		requires(sizeof(parser) != 0)
		{
			if constexpr (requires { parser.make_preparation(ctx); }) {
				parser.make_preparation(ctx);
			}
		}

		template<typename TraverseData, typename Node>
		auto parse(dsl::TraverseContext& ctx, TraverseData& data, Node const* value, type_safe::output_parameter<T> out) const {
			if constexpr (std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>) {
				T tmp;
				if (!parser.parse(ctx, data, value, type_safe::opt_ref(&tmp))) {
					return false;
				}
				out = OV_MOV(tmp);
				return true;
			} else {
				alignas(T) std::array<std::byte, sizeof(T)> bytes;

				static constexpr auto dtor = [](T* ptr) {
					ptr->~T();
				};
				std::unique_ptr<T, decltype(dtor)> result { new (bytes.data()) T };

				if (!parser.parse(ctx, data, value, type_safe::opt_ref(result.get()))) {
					return false;
				}

				if (!result) {
					return false;
				}

				out = OV_MOV(*result);
				return true;
			}
		}
	};

	template<>
	struct ValueParser<ovdl::symbol<>> {
		template<typename TraverseData, typename Node>
		bool parse(
		    dsl::TraverseContext& ctx, TraverseData& data, Node const* value, type_safe::optional_ref<ovdl::symbol<>> in_out
		) const {
			if constexpr (dsl::node_with_value_symbol<Node>) {
				if (in_out.has_value()) {
					in_out.value() = value->value();
				}
				return std::true_type {};
			} else {
				return dsl::visit_flat(
				    ctx, data, value, [this, in_out](dsl::TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					    return parse(ctx, data, fv, in_out);
				    }
				);
			}
		}
	};

	template<>
	struct ValueParser<std::string_view> {
		template<typename TraverseData, typename Node>
		auto parse(
		    dsl::TraverseContext& ctx, TraverseData& data, Node const* value, type_safe::optional_ref<std::string_view> in_out
		) const {
			ovdl::symbol<> symbol;
			if (!ValueParser<ovdl::symbol<>> {}.parse(ctx, data, value, type_safe::opt_ref(symbol))) {
				return false;
			}

			if (in_out.has_value()) {
				in_out.value() = symbol.view();
			}

			return true;
		}
	};

	template<>
	struct ValueParser<fixed_point_t> {
		template<typename TraverseData, typename Node>
		static auto parse(
		    dsl::TraverseContext& ctx, TraverseData& data, Node const* value, type_safe::optional_ref<fixed_point_t> in_out
		) {
			std::string_view sv;
			if (!data.template parser_for<std::string_view>().parse(ctx, data, value, type_safe::opt_ref(sv))) {
				return false;
			}

			fixed_point_t result;
			std::from_chars_result from = fp::from_chars(result, sv.data(), sv.data() + sv.size());
			if (from.ec != std::errc {}) {
				diagnostic::type_conversion_failed(ctx, value, "OpenVic::fixed_point_t");
				return false;
			}

			in_out.value() = OV_MOV(result);
			return true;
		}
	};

	template<string_literal Key, auto Value>
	struct KeyValue {
		static constexpr auto key = Key;
		static constexpr auto value = Value;
	};

	template<string_literal Key, auto Value>
	inline static constexpr auto KV = KeyValue<Key, Value> {};

	template<typename T, KeyValue... KVs>
	struct ValueMapper {
		static constexpr std::integral_constant<std::size_t, sizeof...(KVs)> size {};
		static constexpr std::array<std::string_view, size> keys() {
			return { std::string_view { KVs.key }... };
		}

		std::array<ovdl::symbol<>, size> symbols {};

		void make_preparation(dsl::TraverseContext& ctx) {
			std::size_t i = 0;
			((symbols[i++] = ctx.find_intern(KVs.key)), ...);
		}

		template<typename TraverseData, typename Node>
		auto parse(dsl::TraverseContext& ctx, TraverseData& data, Node const* value, type_safe::optional_ref<T> in_out) const {
			if (OV_unlikely(ctx.parser() == nullptr)) {
				std::string_view sv;
				if (!data.template parser_for<std::string_view>().parse(ctx, data, value, type_safe::opt_ref(sv))) {
					return false;
				}

				bool success = false;
				std::size_t i = 0;
				// first matching key wins
				((sv == KVs.key && (in_out.value() = KVs.value, success = true, true)) || ...);

				if (!success) {
					diagnostic::expected_values(ctx, value, keys());
				}

				return success;
			}

			ovdl::symbol<> symbol;
			if (!data.template parser_for<ovdl::symbol<>>().parse(ctx, data, value, type_safe::opt_ref(symbol))) {
				return false;
			}

			bool success = false;
			std::size_t i = 0;
			// first matching key wins
			((symbol && symbol == symbols[i++] && (in_out.value() = KVs.value, success = true, true)) || ...);

			if (!success) {
				diagnostic::expected_values(ctx, value, keys());
			}

			return success;
		}
	};

	template<>
	struct ValueParser<bool> : ValueMapper<bool, KV<"yes", true>, KV<"no", false>> {};
}
