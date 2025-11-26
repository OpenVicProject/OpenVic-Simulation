#include "FixedPoint.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/core/Typedefs.hpp"

using namespace OpenVic;

fmt::format_context::iterator fmt::formatter<fixed_point_t>::format(fixed_point_t fp, format_context& ctx) const {
	format_specs specs { _specs };
	if (_specs.dynamic()) {
		detail::handle_dynamic_spec(_specs.dynamic_precision(), specs.precision, _specs.precision_ref, ctx);
		detail::handle_dynamic_spec(_specs.dynamic_width(), specs.width, _specs.width_ref, ctx);
	}

	fixed_point_t::stack_string result = fp.to_array(specs.precision);
	if (OV_unlikely(result.empty())) {
		return ctx.out();
	}

	string_view view { result.data(), result.size() };

	sign s = fp.is_negative() ? sign::minus : _specs.sign();

	format_context::iterator out = ctx.out();
	if (_specs.align() == align::numeric && s != sign::none) {
		if (s == sign::minus) {
			view = { view.data() + 1, view.size() - 1 };
		}

		*out++ = detail::getsign<char>(s);
		s = sign::none;
		if (specs.width != 0) {
			--specs.width;
		}
	}
	// Prevents string precision behavior
	specs.precision = -1;

	if (s == sign::none || s == sign::minus) {
		return detail::write<char>(out, view, specs, ctx.locale());
	}

	memory_buffer buffer;
	buffer.resize(view.size() + 1);

	decltype(buffer)::value_type* buf_out = buffer.data();
	*buf_out++ = detail::getsign<char>(s);
	buf_out = detail::write(buf_out, view);

	view = { buffer.data(), buffer.size() };
	return detail::write<char>(out, view, specs, ctx.locale());
}
