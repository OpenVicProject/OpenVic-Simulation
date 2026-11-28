#pragma once

#include <cstdint>

#include <fmt/base.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic::dataloader {
	struct diagnostic_code_t
	    : type_safe::strong_typedef<diagnostic_code_t, std::uint32_t>,
	      type_safe::strong_typedef_op::equality_comparison<diagnostic_code_t>,
	      type_safe::strong_typedef_op::mixed_equality_comparison<diagnostic_code_t, std::uint32_t> {
		using strong_typedef::strong_typedef;
	};
}

template<>
struct fmt::formatter<OpenVic::dataloader::diagnostic_code_t> {
	constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) {
		_specs.set_type(presentation_type::hex);
		if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
			_specs.set_upper();
			return ctx.begin();
		}

		auto begin = ctx.begin();
		auto end = ctx.end();

		auto c = '\0';
		if (end - begin > 1) {
			auto next = fmt::detail::to_ascii(begin[1]);
			c = fmt::detail::parse_align(next) == align::none ? fmt::detail::to_ascii(*begin) : '\0';
		} else {
			c = fmt::detail::to_ascii(*begin);
		}

		// Preserves lowercase behavior if supplied x or b, defaults to uppercase
		switch (c) {
		case 'x': break;
		case 'b': break;
		default:  _specs.set_upper(); break;
		}

		return fmt::detail::parse_format_specs(
		    ctx.begin(),
		    ctx.end(),
		    _specs,
		    ctx,
		    fmt::detail::type_constant<type_safe::underlying_type<OpenVic::dataloader::diagnostic_code_t>, char>::value
		);
	}

	fmt::format_context::iterator format(OpenVic::dataloader::diagnostic_code_t value, fmt::format_context& ctx) const;

private:
	fmt::detail::dynamic_format_specs<char> _specs;
};
