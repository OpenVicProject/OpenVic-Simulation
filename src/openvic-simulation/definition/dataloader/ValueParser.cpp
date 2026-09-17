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
#include "openvic-simulation/definition/dataloader/ErrorMacros.hpp"
#include "openvic-simulation/definition/dataloader/Logger.hpp"
#include "openvic-simulation/definition/dataloader/TreeTraverse.hpp"
#include "openvic-simulation/definition/dataloader/Utility.hpp"

using namespace OpenVic;
using namespace OpenVic::dataloader;

Error ValueExtractor<emptyable<ovdl::symbol<>>>::extract(ValueExtractorArguments<emptyable<ovdl::symbol<>>> args) {
	if (auto* fv = dryad::node_try_cast<ovdl::v2script::ast::FlatValue>(args.node)) {
		args.out.value = fv->value();
		return Error::OK;
	}

	OV_DL_ERR_FAIL_V_MSG(
	    Error::FAILED,
	    make_location_message(
	        args.parser, args.node, "Expected a string, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	    )
	);
}

Error ValueExtractor<ovdl::symbol<>>::extract(ValueExtractorArguments<ovdl::symbol<>> args) {
	emptyable<ovdl::symbol<>> e { args.out };
	OV_RETURN_IF_ERROR(ValueExtractor<emptyable<ovdl::symbol<>>>::extract({ args.parser, args.node, e }));

	ovdl::symbol<> symbol = args.out;
	args.out = {};
	OV_DL_ERR_FAIL_COND_V_MSG(
	    symbol.view().empty(), Error::FAILED, make_location_message(args.parser, args.node, "Unexpected empty string")
	);
	args.out = symbol;

	return Error::OK;
}

Error ValueExtractor<emptyable<std::string_view>>::extract(ValueExtractorArguments<emptyable<std::string_view>> args) {
	ovdl::symbol<> symbol;
	emptyable<ovdl::symbol<>> e { symbol };
	OV_RETURN_IF_ERROR(ValueExtractor<emptyable<ovdl::symbol<>>>::extract({ args.parser, args.node, e }));
	args.out.value = symbol.view();
	return Error::OK;
}


Error ValueExtractor<std::string_view>::extract(ValueExtractorArguments<std::string_view> args) {
	ovdl::symbol<> symbol;
	OV_RETURN_IF_ERROR(ValueExtractor<ovdl::symbol<>>::extract({ args.parser, args.node, symbol }));
	args.out = symbol.view();
	return Error::OK;
}

Error ValueExtractor<int_bool>::extract(ValueExtractorArguments<int_bool> args) {
	uint64_t out;
	OV_RETURN_IF_ERROR(ValueExtractor<uint64_t>::extract({ args.parser, args.node, out }));

	if (out > 1) {
		log::warn(make_location_message(args.parser, args.node, "Found integer bool with value {} instead of 0 or 1", out));
	}

	args.out.value = out != 0;
	return Error::OK;
}

Error ValueExtractor<fixed_point_t>::extract(ValueExtractorArguments<fixed_point_t> args) {
	std::string_view sv;

	OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.parser, args.node, sv }));

	fixed_point_t f;
	std::from_chars_result result = fp::from_chars_with_plus(f, sv.data(), sv.data() + sv.size());
	if (result.ec == std::errc {}) {
		args.out = f;
		return Error::OK;
	}

	OV_DL_ERR_FAIL_V_MSG(
	    Error::FAILED, make_location_message(args.parser, args.node, "Expected a fixed point value, found {}", sv)
	);
}

template<typename T>
Error ValueExtractor<vec2_t<T>>::extract(ValueExtractorArguments<vec2_t<T>> args) {
	if (auto* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node)) {
		return Traverse.options<{ .unknown_level = spdlog::level::err }>()
		    .template expect_once<"x">(args.out.x)
		    .template expect_once<"y">(args.out.y)(*args.parser, lv);
	}

	OV_DL_ERR_FAIL_V_MSG(
	    Error::FAILED,
	    make_location_message(
	        args.parser, args.node, "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	    )
	);
}

template<typename T>
Error ValueExtractor<vec3_t<T>>::extract(ValueExtractorArguments<vec3_t<T>> args) {
	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	OV_DL_ERR_FAIL_NULL_V_MSG(
	    lv,
	    Error::FAILED,
	    make_location_message(
	        args.parser, args.node, "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	    )
	);

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index >= 3) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		OV_DL_ERR_CONTINUE_MSG(
		    value == nullptr,
		    make_location_message(
		        args.parser,
		        sub_node,
		        "Expected a value statement, found {}",
		        ovdl::v2script::ast::get_kind_name(sub_node->kind())
		    )
		);

		T tmp;
		if (ValueExtractor<T>::extract({ args.parser, value->value(), tmp }) != Error::OK) {
			continue;
		}
		args.out[index] = tmp;
	}

	OV_DL_ERR_FAIL_COND_V_MSG(
	    count >= 3, Error::FAILED, make_location_message(args.parser, args.node, "Expected 3 values in list, found {}", count)
	);

	return Error::OK;
}

template<typename T>
Error ValueExtractor<vec4_t<T>>::extract(ValueExtractorArguments<vec4_t<T>> args) {
	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	OV_DL_ERR_FAIL_NULL_V_MSG(
	    lv,
	    Error::FAILED,
	    make_location_message(
	        args.parser, args.node, "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	    )
	);

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index >= 4) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		OV_DL_ERR_CONTINUE_MSG(
		    value == nullptr,
		    make_location_message(
		        args.parser,
		        sub_node,
		        "Expected a value statement, found {}",
		        ovdl::v2script::ast::get_kind_name(sub_node->kind())
		    )
		);

		T tmp;
		if (ValueExtractor<T>::extract({ args.parser, value->value(), tmp }) != Error::OK) {
			continue;
		}
		args.out[index] = tmp;
	}

	OV_DL_ERR_FAIL_COND_V_MSG(
	    count >= 4, Error::FAILED, make_location_message(args.parser, args.node, "Expected 4 values in list, found {}", count)
	);

	return Error::OK;
}

Error ValueExtractor<years>::extract(ValueExtractorArguments<years> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.parser, args.node, value }));
	args.out.value = Timespan::from_years(value);
	return Error::OK;
}

Error ValueExtractor<months>::extract(ValueExtractorArguments<months> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.parser, args.node, value }));
	args.out.value = Timespan::from_months(value);
	return Error::OK;
}

Error ValueExtractor<days>::extract(ValueExtractorArguments<days> args) {
	Timespan::value_t value;
	OV_RETURN_IF_ERROR(ValueExtractor<Timespan::value_t>::extract({ args.parser, args.node, value }));
	args.out.value = Timespan::from_days(value);
	return Error::OK;
}

// TODO: Allow providing spdlog::logger* to Date::from_string_log
Error ValueExtractor<Date>::extract(ValueExtractorArguments<Date> args) {
	std::string_view sv;

	OV_RETURN_IF_ERROR(ValueExtractor<std::string_view>::extract({ args.parser, args.node, sv }));

	Date date;
	Date::from_chars_result result = date.from_chars(sv.data(), sv.data() + sv.size());

	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::invalid_argument && result.type == Date::errc_type::year && result.ptr == result.type_first,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Could not parse year value")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::value_too_large && result.type == Date::errc_type::year,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Year value was too large or too small")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::result_out_of_range && result.type == Date::errc_type::year,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Only year value could be found")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::invalid_argument && result.type == Date::errc_type::year && result.ptr != result.type_first,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Year value was missing a separator (\"{}\")", Date::SEPARATOR_CHARACTER)
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::invalid_argument && result.type == Date::errc_type::month && result.ptr == result.type_first,
	    Error::FAILED,
	    "Could not parse month value."
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::not_supported && result.type == Date::errc_type::month,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Month value cannot be 0")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::value_too_large && result.type == Date::errc_type::month && result.ptr == result.type_first,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Month value cannot be larger than {}", Date::MONTHS_IN_YEAR)
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::result_out_of_range && result.type == Date::errc_type::month,
	    Error::FAILED,
	    "Only year and month value could be found."
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::invalid_argument && result.type == Date::errc_type::month && result.ptr != result.type_first,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Month value was missing a separator (\"{}\")", Date::SEPARATOR_CHARACTER)
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::invalid_argument && result.type == Date::errc_type::day && result.ptr == result.type_first,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Could not parse day value")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::not_supported && result.type == Date::errc_type::day,
	    Error::FAILED,
	    make_location_message(args.parser, args.node, "Day value cannot be 0")
	);
	OV_DL_ERR_FAIL_COND_V_MSG(
	    result.ec == std::errc::value_too_large && result.type == Date::errc_type::day && result.ptr == result.type_first,
	    Error::FAILED,
	    make_location_message(
	        args.parser,
	        args.node,
	        "Day value cannot be larger than {} for {}",
	        Date::DAYS_IN_MONTH[date.get_month() - 1],
	        date.get_month()
	    )
	);

	return Error::OK;
}

template<typename ValueT, typename ColourIntT, typename ColourTraits>
Error ValueExtractor<basic_colour_t<ValueT, ColourIntT, ColourTraits>>::extract(
    ValueExtractorArguments<basic_colour_t<ValueT, ColourIntT, ColourTraits>> args
) {
	using colour_t = basic_colour_t<ValueT, ColourIntT, ColourTraits>;

	auto const* lv = dryad::node_try_cast<ovdl::v2script::ast::ListValue>(args.node);
	OV_DL_ERR_FAIL_NULL_V_MSG(
	    lv,
	    Error::FAILED,
	    make_location_message(
	        args.parser, args.node, "Expected a list value, found {}", ovdl::v2script::ast::get_kind_name(args.node->kind())
	    )
	);

	size_t count;
	auto statements = lv->statements();
	for (auto [index, sub_node] : statements | ranges::views::enumerate) {
		count = index + 1;
		if (index >= 3) {
			continue;
		}

		auto const* value = dryad::node_try_cast<ovdl::v2script::ast::ValueStatement>(sub_node);
		OV_DL_ERR_CONTINUE_MSG(
		    value == nullptr,
		    make_location_message(
		        args.parser,
		        sub_node,
		        "Expected a value statement, found {}",
		        ovdl::v2script::ast::get_kind_name(sub_node->kind())
		    )
		);

		fixed_point_t tmp;
		if (ValueExtractor<fixed_point_t>::extract({ args.parser, value->value(), tmp }) != Error::OK) {
			continue;
		}

		OV_DL_ERR_CONTINUE_MSG(
		    tmp < 0 || tmp > 255,
		    make_location_message(
		        args.parser,
		        value->value(),
		        "Expected color component fractional between 0 and 1 or integer between 0 and 255, found {}",
		        tmp
		    )
		);

		auto trunc = tmp.truncate<typename colour_t::value_type>();
		if (tmp <= 1) {
			tmp *= 255;
		} else if (!tmp.is_negative()) {
			log::warn(make_location_message(
			    args.parser,
			    value->value(),
			    "Expected color component fractional between 0 and 1 or integer between 0 and 255, found fractional {}, "
			    "truncating to {}",
			    tmp,
			    trunc
			));
		}
		args.out[index] = trunc;
	}

	OV_DL_ERR_FAIL_COND_V_MSG(
	    count >= 3, Error::FAILED, make_location_message(args.parser, args.node, "Expected 3 values in list, found {}", count)
	);

	return Error::OK;
}

template<typename ValueT, typename ColourIntT, typename ColourTraits>
Error ValueExtractor<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>>::extract(
    ValueExtractorArguments<hex<basic_colour_t<ValueT, ColourIntT, ColourTraits>>> args
) {
	using colour_t = basic_colour_t<ValueT, ColourIntT, ColourTraits>;

	typename colour_t::integer_type integer;
	hex<typename colour_t::integer_type> out { integer };
	OV_RETURN_IF_ERROR((ValueExtractor<hex<typename colour_t::integer_type>>::extract({ args.parser, args.node, out })));

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
