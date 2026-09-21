#include "ValueParser.hpp"

#include <charconv>
#include <string_view>
#include <system_error>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <range/v3/view/enumerate.hpp>

#include <spdlog/common.h>

#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/core/object/Colour.hpp"
#include "openvic-simulation/core/object/Date.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/core/object/FixedPoint/String.hpp"
#include "openvic-simulation/core/object/Timespan.hpp"
#include "openvic-simulation/core/object/Vector.hpp"
#include "openvic-simulation/definition/dataloader/TreeTraverse.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

using namespace OpenVic;
using namespace OpenVic::dataloader;

Error ValueExtractor<emptyable<ovdl::symbol<>>>::extract(ValueExtractorArguments<emptyable<ovdl::symbol<>>> args) {
	if (auto* fv = dryad::node_try_cast<ovdl::v2script::ast::FlatValue>(args.node)) {
		args.out.value = fv->value();
		return Error::OK;
	}

	args.traverse.diagnostics.error(args.node).with_message(
	    "Expected a string, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	);
	return Error::FAILED;
}

Error ValueExtractor<ovdl::symbol<>>::extract(ValueExtractorArguments<ovdl::symbol<>> args) {
	ovdl::symbol<> symbol;
	emptyable<ovdl::symbol<>> e { symbol };
	OV_RETURN_IF_ERROR(ValueExtractor<emptyable<ovdl::symbol<>>>::extract({ args.traverse, args.node, e }));

	if (symbol.view().empty()) {
		args.traverse.diagnostics.error(args.node).with_message("Unexpected empty string");
		return Error::FAILED;
	}
	args.out = symbol;

	return Error::OK;
}

Error ValueExtractor<emptyable<std::string_view>>::extract(ValueExtractorArguments<emptyable<std::string_view>> args) {
	ovdl::symbol<> symbol;
	emptyable<ovdl::symbol<>> e { symbol };
	OV_RETURN_IF_ERROR(ValueExtractor<emptyable<ovdl::symbol<>>>::extract({ args.traverse, args.node, e }));
	args.out.value = symbol.view();
	return Error::OK;
}


Error ValueExtractor<std::string_view>::extract(ValueExtractorArguments<std::string_view> args) {
	ovdl::symbol<> symbol;
	OV_RETURN_IF_ERROR(ValueExtractor<ovdl::symbol<>>::extract({ args.traverse, args.node, symbol }));
	args.out = symbol.view();
	return Error::OK;
}

Error ValueExtractor<int_bool>::extract(ValueExtractorArguments<int_bool> args) {
	uint64_t out;
	OV_RETURN_IF_ERROR(ValueExtractor<uint64_t>::extract({ args.traverse, args.node, out }));

	if (out > 1) {
		args.traverse.diagnostics.warning(args.node).with_message("Found integer bool with value {} instead of 0 or 1", out);
	}

	args.out.value = out != 0;
	return Error::OK;
}

Error ValueExtractor<fixed_point_t>::extract(ValueExtractorArguments<fixed_point_t> args) {
	std::string_view sv;
	OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.traverse, args.node, sv }));

	fixed_point_t f;
	std::from_chars_result result = fp::from_chars_with_plus(f, sv.data(), sv.data() + sv.size());
	if (result.ec == std::errc {}) {
		args.out = f;
		return Error::OK;
	}

	args.traverse.diagnostics.error(args.node).with_message("Expected a fixed point value, found {}", sv);
	return Error::FAILED;
}

template<typename T>
Error ValueExtractor<vec2_t<T>>::extract(ValueExtractorArguments<vec2_t<T>> args) {
	if (auto* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node)) {
		return Traverse.options<{ .unknown_level = DiagnosticLevel::ERROR }>()
		    .template expect_once<"x">(args.out.x)
		    .template expect_once<"y">(args.out.y)(args.traverse, lv)
		    .error;
	}

	args.traverse.diagnostics.error(args.node).with_message(
	    "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	);
	return Error::FAILED;
}

template<typename T>
Error ValueExtractor<vec3_t<T>>::extract(ValueExtractorArguments<vec3_t<T>> args) {
	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	if (lv == nullptr) {
		args.traverse.diagnostics.error(args.node).with_message(
		    "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
		);
		return Error::FAILED;
	}

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index > 3) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		if (value == nullptr) {
			args.traverse.diagnostics.error(sub_node).with_message(
			    "Expected a value statement, found {}", ovdl::v2script::ast::get_kind_name(sub_node->kind())
			);
			continue;
		}

		T tmp;
		if (ValueExtractor<T>::extract({ args.traverse, value->value(), tmp }) != Error::OK) {
			continue;
		}
		args.out[index] = tmp;
	}

	if (count != 3) {
		args.traverse.diagnostics.error(args.node).with_message("Expected 3 values in list, found {}", count);
		return Error::FAILED;
	}

	return Error::OK;
}

template<typename T>
Error ValueExtractor<vec4_t<T>>::extract(ValueExtractorArguments<vec4_t<T>> args) {
	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	if (lv == nullptr) {
		args.traverse.diagnostics.error(args.node).with_message(
		    "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
		);
		return Error::FAILED;
	}

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index > 4) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		if (value == nullptr) {
			args.traverse.diagnostics.error(sub_node).with_message(
			    "Expected a value statement, found {}", ovdl::v2script::ast::get_kind_name(sub_node->kind())
			);
			continue;
		}

		T tmp;
		if (ValueExtractor<T>::extract({ args.traverse, value->value(), tmp }) != Error::OK) {
			continue;
		}
		args.out[index] = tmp;
	}

	if (count != 4) {
		args.traverse.diagnostics.error(args.node).with_message("Expected 4 values in list, found {}", count);
		return Error::FAILED;
	}

	return Error::OK;
}

Error ValueExtractor<years>::extract(ValueExtractorArguments<years> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.traverse, args.node, value }));
	args.out.value = Timespan::from_years(value);
	return Error::OK;
}

Error ValueExtractor<months>::extract(ValueExtractorArguments<months> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.traverse, args.node, value }));
	args.out.value = Timespan::from_months(value);
	return Error::OK;
}

Error ValueExtractor<days>::extract(ValueExtractorArguments<days> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.traverse, args.node, value }));
	args.out.value = Timespan::from_days(value);
	return Error::OK;
}

// TODO: Allow providing spdlog::logger* to Date::from_string_log
Error ValueExtractor<Date>::extract(ValueExtractorArguments<Date> args) {
	std::string_view sv;

	OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.traverse, args.node, sv }));

	Date date;
	Date::from_chars_result result = date.from_chars(sv.data(), sv.data() + sv.size());
	if (result.ec == std::errc {}) {
		args.out = date;
		return Error::OK;
	}

	args.traverse.diagnostics.error(args.node).with_message("Expected a date value, found {}", sv);
	return Error::FAILED;
}

template<typename ValueT, typename ColourIntT, typename ColourTraits>
Error ValueExtractor<basic_colour_t<ValueT, ColourIntT, ColourTraits>>::extract(
    ValueExtractorArguments<basic_colour_t<ValueT, ColourIntT, ColourTraits>> args
) {
	using colour_t = basic_colour_t<ValueT, ColourIntT, ColourTraits>;

	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	if (lv == nullptr) {
		args.traverse.diagnostics.error(args.node).with_message(
		    "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
		);
		return Error::FAILED;
	}

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index >= 3) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		if (value == nullptr) {
			args.traverse.diagnostics.error(sub_node).with_message(
			    "Expected a value statement, found {}", ovdl::v2script::ast::get_kind_name(sub_node->kind())
			);
			continue;
		}

		fixed_point_t tmp;
		if (ValueExtractor<fixed_point_t>::extract({ args.traverse, value->value(), tmp }) != Error::OK) {
			continue;
		}

		if (tmp < 0 || tmp > 255) {
			args.traverse.diagnostics.error(value->value())
			    .with_message(
			        "Expected color component fractional between 0 and 1 or integer between 0 and 255, found {}", tmp
			    );
			continue;
		}

		auto trunc = tmp.truncate<typename colour_t::value_type>();
		if (tmp <= 1) {
			tmp *= 255;
		} else if (!tmp.is_negative()) {
			args.traverse.diagnostics.warning(value->value())
			    .with_message(
			        "Expected color component fractional between 0 and 1 or integer between 0 and 255, found fractional {}, "
			        "truncating to {}",
			        tmp,
			        trunc
			    );
		}
		args.out[index] = trunc;
	}

	if (count != 3) {
		args.traverse.diagnostics.error(args.node).with_message("Expected 3 values in list, found {}", count);
		return Error::FAILED;
	}

	return Error::OK;
}

template<typename ValueT, typename ColourIntT, typename ColourTraits>
Error ValueExtractor<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>>::extract(
    ValueExtractorArguments<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>> args
) {
	using colour_t = basic_colour_t<ValueT, ColourIntT, ColourTraits>;

	typename colour_t::integer_type integer;
	hex<typename colour_t::integer_type> out { integer };
	OV_RETURN_IF_ERROR((ValueExtractor<hex<typename colour_t::integer_type>>::extract({ args.traverse, args.node, out })));

	args.out.value = colour_t::from_argb(integer);
	return Error::OK;
}

template struct OpenVic::dataloader::ValueExtractor<OpenVic::fvec2_t>;
template struct OpenVic::dataloader::ValueExtractor<OpenVic::ivec2_t>;

template struct OpenVic::dataloader::ValueExtractor<OpenVic::fvec3_t>;
template struct OpenVic::dataloader::ValueExtractor<OpenVic::ivec3_t>;

template struct OpenVic::dataloader::ValueExtractor<OpenVic::fvec4_t>;
template struct OpenVic::dataloader::ValueExtractor<OpenVic::ivec4_t>;

template struct OpenVic::dataloader::ValueExtractor<OpenVic::colour_rgb_t>;
template struct OpenVic::dataloader::ValueExtractor<OpenVic::colour_argb_t>;

template struct OpenVic::dataloader::ValueExtractor<OpenVic::dataloader::hex<OpenVic::colour_rgb_t>>;
template struct OpenVic::dataloader::ValueExtractor<OpenVic::dataloader::hex<OpenVic::colour_argb_t>>;
