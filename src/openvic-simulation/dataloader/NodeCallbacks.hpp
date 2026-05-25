#pragma once

#include <function2/function2.hpp>

#include "openvic-simulation/core/template/FunctionalConcepts.hpp"
#include "openvic-simulation/dataloader/Node_forwarded.hpp"

namespace OpenVic {
	namespace NodeTools {
		template<typename... Args>
		using callback_t = fu2::function_base<true, true, fu2::capacity_default, false, false, bool(Args...)>;

		using node_callback_t = callback_t<ovdl::v2script::ast::Node const*>;

		template<typename Fn>
		concept NodeCallback = Callback<Fn, ovdl::v2script::ast::Node const*>;

		template<typename Fn>
		concept KeyValueCallback = Callback<Fn, std::string_view, ovdl::v2script::ast::Node const*>;

		template<typename Fn, typename Map>
		concept MapKeyValueCallback = Callback<Fn, Map const&, std::string_view, ovdl::v2script::ast::Node const*>;
	}
}