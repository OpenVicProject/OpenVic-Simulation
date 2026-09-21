#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <fmt/ranges.h>

#include <type_safe/output_parameter.hpp>
#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/core/string/CharConv.hpp"
#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/core/ui/TextFormat.hpp"
#include "openvic-simulation/definition/dataloader/TraverseResult.hpp"

namespace OpenVic {
	class fixed_point_t;

	template<typename T>
	struct vec2_t;
	template<typename T>
	struct vec3_t;
	template<typename T>
	struct vec4_t;

	class Timespan;
	class Date;

	template<typename ValueT, typename IntT, bool HasAlpha>
	struct colour_traits;

	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	class basic_colour_t;

	enum class text_format_t : uint8_t;
}

namespace OpenVic::dataloader {
	template<typename T = std::string_view>
	struct strict_id {
		T& value;
	};

	template<typename T = std::string_view>
	struct strict_string {
		T& value;
	};

	template<typename T = std::string_view>
	struct emptyable {
		T& value;
	};

	struct int_bool {
		bool& value;
	};

	template<typename T, std::size_t Base>
	struct base {
		static constexpr std::integral_constant<std::size_t, Base> base_value = {};
		T& value;
	};

	template<typename T>
	using binary = base<T, 2>;

	template<typename T>
	using octal = base<T, 8>;

	template<typename T>
	using decimal = base<T, 10>;

	template<typename T>
	using hex = base<T, 16>;

	struct years {
		Timespan& value;
	};

	struct months {
		Timespan& value;
	};

	struct days {
		Timespan& value;
	};

	template<typename T>
	struct overwrite_optional : std::optional<T> {
		using base_type = std::optional<T>;
		using base_type::base_type;
	};

	template<string_literal Key, auto Value>
	struct KeyValue {
		static constexpr auto key = Key;
		static constexpr auto value = Value;
	};

	template<string_literal Key, auto Value>
	inline static constexpr auto KV = KeyValue<Key, Value> {};

	template<typename T, KeyValue... KVs>
	struct ValueMapper;

	template<typename T>
	struct ValueInitializeArguments {
		TraverseResult& traverse;
		T& out;
	};

	template<typename T>
	struct ValueExtractorArguments {
		TraverseResult& traverse;
		ovdl::v2script::ast::Value const* node;
		T& out;
	};

	template<typename T>
	struct ValueExtractorArguments<type_safe::output_parameter<T>> {
		TraverseResult& traverse;
		ovdl::v2script::ast::Value const* node;
		type_safe::output_parameter<T> out;
	};

	template<typename T>
	struct ValueExtractorArguments<T&> {
		TraverseResult& traverse;
		ovdl::v2script::ast::Value const* node;
		T& out;
	};

	template<typename T>
	struct ValueFinalizeArguments {
		TraverseResult& traverse;
		T& out;
	};

	template<typename T>
	struct ValueExtractor;

	template<>
	struct ValueExtractor<emptyable<ovdl::symbol<>>> {
		static Error extract(ValueExtractorArguments<emptyable<ovdl::symbol<>>> args);
	};

	template<>
	struct ValueExtractor<ovdl::symbol<>> {
		static Error extract(ValueExtractorArguments<ovdl::symbol<>> args);
	};

	template<>
	struct ValueExtractor<emptyable<std::string_view>> {
		static Error extract(ValueExtractorArguments<emptyable<std::string_view>> args);
	};

	template<>
	struct ValueExtractor<std::string_view> {
		static Error extract(ValueExtractorArguments<std::string_view> args);
	};

	template<std::integral T>
	struct ValueExtractor<T> {
		static Error extract(ValueExtractorArguments<T> args) {
			std::string_view sv;
			OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.traverse, args.node, sv }));

			int64_t tmp;
			std::from_chars_result result = from_chars(sv.data(), sv.data() + sv.size(), tmp);
			if (result.ec == std::errc {}) {
				args.out = tmp;
				return Error::OK;
			}

			if (result.ec == std::errc::result_out_of_range) {
				args.traverse.diagnostics.error(args.node).with_message("Overflow of integer, found {}", sv);
			} else {
				args.traverse.diagnostics.error(args.node).with_message("Expected an integer, found {}", sv);
			}
			return Error::FAILED;
		}
	};

	template<std::integral T>
	struct ValueExtractor<base<T, 10>> {
		static Error extract(ValueExtractorArguments<base<T, 10>> args) {
			return ValueExtractor<T>::extract({ args.traverse, args.node, args.out.value });
		}
	};

	template<std::integral T, std::size_t Base>
	struct ValueExtractor<base<T, Base>> {
		static Error extract(ValueExtractorArguments<base<T, Base>> args) {
			std::string_view sv;
			OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.traverse, args.node, sv }));

			T tmp;
			std::from_chars_result result = from_chars(sv.data(), sv.data() + sv.size(), tmp, Base);
			if (result.ec == std::errc {}) {
				args.out.value = tmp;
				return Error::OK;
			}

			if (result.ec == std::errc::result_out_of_range) {
				args.traverse.diagnostics.error(args.node).with_message("Overflow of integer, found {}", sv);
			} else {
				args.traverse.diagnostics.error(args.node).with_message("Expected an integer, found {}", sv);
			}
			return Error::FAILED;
		}
	};

	template<typename T>
	struct ValueExtractor<strict_id<T>> {
		static Error extract(ValueExtractorArguments<strict_id<T>> args) {
			if (args.node->kind != ovdl::v2script::ast::NodeKind::IdentifierValue) {
				args.traverse.diagnostics.error(args.node).with_message(
				    "Expected identifier, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind)
				);
				return Error::OK;
			}

			return ValueExtractor<T>::extract({ args.traverse, args.node, args.out.value });
		}
	};

	template<typename T>
	struct ValueExtractor<strict_string<T>> {
		static Error extract(ValueExtractorArguments<strict_string<T>> args) {
			if (args.node->kind != ovdl::v2script::ast::NodeKind::StringValue) {
				args.traverse.diagnostics.error(args.node).with_message(
				    "Expected string, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind)
				);
				return Error::OK;
			}

			return ValueExtractor<T>::extract({ args.traverse, args.node, args.out.value });
		}
	};

	template<typename T>
	struct ValueExtractor<strict_string<emptyable<T>>> {
		static Error extract(ValueExtractorArguments<strict_string<emptyable<T>>> args) {
			if (args.node->kind != ovdl::v2script::ast::NodeKind::StringValue) {
				args.traverse.diagnostics.error(args.node).with_message(
				    "Expected string, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind)
				);
				return Error::OK;
			}

			return ValueExtractor<emptyable<T>>::extract({ args.traverse, args.node, args.out.value });
		}
	};

	template<typename T>
	struct ValueExtractor<emptyable<strict_string<T>>> {
		static Error extract(ValueExtractorArguments<emptyable<strict_string<T>>> args) {
			if (args.node->kind != ovdl::v2script::ast::NodeKind::StringValue) {
				args.traverse.diagnostics.error(args.node).with_message(
				    "Expected string, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind)
				);
				return Error::OK;
			}

			emptyable<T> empty { args.out.value.value };
			return ValueExtractor<emptyable<T>>::extract({ args.traverse, args.node, empty });
		}
	};

	template<>
	struct ValueExtractor<int_bool> {
		static Error extract(ValueExtractorArguments<int_bool> args);
	};

	template<derived_from_specialization_of<type_safe::strong_typedef> T>
	struct ValueExtractor<T> {
		static Error extract(ValueExtractorArguments<T> args) {
			using underlying_type = type_safe::underlying_type<T>;

			return ValueExtractor<underlying_type>::extract(
			    { args.traverse, args.node, static_cast<underlying_type&>(args.out) }
			);
		}
	};

	template<>
	struct ValueExtractor<fixed_point_t> {
		static Error extract(ValueExtractorArguments<fixed_point_t> args);
	};

	template<typename T>
	struct ValueExtractor<vec2_t<T>> {
		static Error extract(ValueExtractorArguments<vec2_t<T>> args);
	};

	template<typename T>
	struct ValueExtractor<vec3_t<T>> {
		static Error extract(ValueExtractorArguments<vec3_t<T>> args);
	};

	template<typename T>
	struct ValueExtractor<vec4_t<T>> {
		static Error extract(ValueExtractorArguments<vec4_t<T>> args);
	};

	template<>
	struct ValueExtractor<years> {
		static Error extract(ValueExtractorArguments<years> args);
	};

	template<>
	struct ValueExtractor<months> {
		static Error extract(ValueExtractorArguments<months> args);
	};

	template<>
	struct ValueExtractor<days> {
		static Error extract(ValueExtractorArguments<days> args);
	};

	template<>
	struct ValueExtractor<Date> {
		static Error extract(ValueExtractorArguments<Date> args);
	};

	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct ValueExtractor<basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		static Error extract(ValueExtractorArguments<basic_colour_t<ValueT, ColourIntT, ColourTraits>> args);
	};

	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct ValueExtractor<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>> {
		static Error extract(ValueExtractorArguments<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>> args);
	};

	template<typename T>
	struct ValueExtractor<std::optional<T>> {
		static Error extract(ValueExtractorArguments<std::optional<T>> args) {
			if (args.out.has_value()) {
				args.traverse.diagnostics.error(args.node).with_message("Unexpected overwriting of set value");
				return Error::OK;
			}

			T tmp;
			OV_RETURN_IF_ERROR(ValueExtractor<T>::extract({ args.traverse, args.node, tmp }));
			args.out = tmp;
			return Error::OK;
		}
	};

	template<typename T>
	struct ValueExtractor<overwrite_optional<T>> {
		static Error extract(ValueExtractorArguments<overwrite_optional<T>> args) {
			T tmp;
			OV_RETURN_IF_ERROR(ValueExtractor<T>::extract({ args.traverse, args.node, tmp }));
			args.out = tmp;
			return Error::OK;
		}
	};

	template<typename T, KeyValue... KVs>
	struct ValueMapper {
		static constexpr std::size_t size = sizeof...(KVs);

		struct SymbolTable {
			ovdl::v2script::Parser const* parser = nullptr;
			std::array<ovdl::symbol<>, size> symbols {};
		};

		inline static thread_local SymbolTable table;

		static constexpr std::array<std::string_view, size> keys() {
			return { std::string_view { KVs.key }... };
		}

		static Error initialize(ValueInitializeArguments<T> args) {
			if (args.traverse.diagnostics.parser() != table.parser) {
				table.parser = args.traverse.diagnostics.parser();
			}

			if (OV_unlikely(args.traverse.diagnostics.parser() == nullptr)) {
				return Error::OK;
			}

			std::size_t i = 0;
			((table.symbols[i++] = args.traverse.diagnostics.parser()->find_intern(KVs.key)), ...);
			return Error::OK;
		}

		static Error extract(ValueExtractorArguments<T> args) {
			if (OV_unlikely(args.traverse.diagnostics.parser() == nullptr)) {
				std::string_view sv;
				OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.traverse, args.node, sv }));

				Error err = Error::FAILED;
				std::size_t i = 0;
				// first matching key wins
				((sv && sv == KVs.key && (args.out = KVs.value, err = Error::OK, true)) || ...);

				if (err != Error::OK) {
					if constexpr (size == 1) {
						args.traverse.diagnostics.error(args.node).with_message(
						    "Expected value: [{}], found {}", keys()[0], sv
						);
					} else {
						args.traverse.diagnostics.error(args.node).with_message(
						    "Expected one of values: [{}], found {}", fmt::join(keys(), ", "), sv
						);
					}
					return err;
				}

				return Error::OK;
			}

			ovdl::symbol<> value;
			OV_RETURN_IF_ERROR(ValueExtractor<ovdl::symbol<>>::extract({ args.traverse, args.node, value }));

			if (OV_unlikely(args.traverse.diagnostics.parser() != table.parser)) {
				initialize({ args.traverse, args.out });
			}

			Error err = Error::FAILED;
			std::size_t i = 0;
			// first matching key wins
			((value && value == table.symbols[i++] && (args.out = KVs.value, err = Error::OK, true)) || ...);

			if (err != Error::OK) {
				if constexpr (size == 1) {
					args.traverse.diagnostics.error(args.node).with_message(
					    "Expected value: [{}], found {}", keys()[0], value.view()
					);
				} else {
					args.traverse.diagnostics.error(args.node).with_message(
					    "Expected one of values: [{}], found {}", fmt::join(keys(), ", "), value.view()
					);
				}
				return err;
			}

			return Error::OK;
		}
	};

	template<>
	struct ValueExtractor<bool> : ValueMapper<bool, KV<"yes", true>, KV<"no", false>> {};

	template<>
	struct ValueExtractor<text_format_t>
	    : ValueMapper<
	          text_format_t,
	          KV<"left", text_format_t::left>,
	          KV<"right", text_format_t::right>,
	          KV<"justified", text_format_t::justified>,
	          KV<"center", text_format_t::centre>,
	          KV<"centre", text_format_t::centre>> {};
}

extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec2_t<OpenVic::fixed_point_t>>;
extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec2_t<int32_t>>;

extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec3_t<OpenVic::fixed_point_t>>;
extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec3_t<int32_t>>;

extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec4_t<OpenVic::fixed_point_t>>;
extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::vec4_t<int32_t>>;

extern template struct OpenVic::dataloader::ValueExtractor<
    OpenVic::basic_colour_t<std::uint8_t, std::uint32_t, OpenVic::colour_traits<std::uint8_t, std::uint32_t, false>>>;
extern template struct OpenVic::dataloader::ValueExtractor<
    OpenVic::basic_colour_t<std::uint8_t, std::uint32_t, OpenVic::colour_traits<std::uint8_t, std::uint32_t, true>>>;

extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::dataloader::hex<
    OpenVic::basic_colour_t<std::uint8_t, std::uint32_t, OpenVic::colour_traits<std::uint8_t, std::uint32_t, false>>>>;
extern template struct OpenVic::dataloader::ValueExtractor<OpenVic::dataloader::hex<
    OpenVic::basic_colour_t<std::uint8_t, std::uint32_t, OpenVic::colour_traits<std::uint8_t, std::uint32_t, true>>>>;
