#include "DiagnosticCode.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

using namespace OpenVic::dataloader;

fmt::format_context::iterator fmt::formatter<diagnostic_code_t>::format(
    diagnostic_code_t value, fmt::format_context& ctx
) const {
	if (!_specs.dynamic()) {
		return fmt::detail::write<char>(ctx.out(), type_safe::get(value), _specs, ctx.locale());
	}
	auto specs = fmt::format_specs(_specs);
	fmt::detail::handle_dynamic_spec(specs.dynamic_width(), specs.width, _specs.width_ref, ctx);
	fmt::detail::handle_dynamic_spec(specs.dynamic_precision(), specs.precision, _specs.precision_ref, ctx);
	return fmt::detail::write<char>(ctx.out(), type_safe::get(value), specs, ctx.locale());
}
