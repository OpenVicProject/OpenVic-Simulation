#pragma once

#include <iterator>
#include <utility>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <fmt/base.h>

#include "openvic-simulation/core/memory/Formatting.hpp"

namespace OpenVic::dataloader {
	template<typename... T>
	memory::string make_location_message(
	    ovdl::v2script::Parser const* parser, ovdl::v2script::ast::Node const* node, fmt::format_string<T...> fmt, T&&... args
	) {
		memory::fmt::basic_memory_buffer<char, 250> result {};
		auto out = std::back_inserter(result);
		if (parser) {
			ovdl::FilePosition pos = parser->get_position(node);
			out = fmt::format_to(out, "{}:{}:{}: ", parser->get_file_path(), pos.start_line, pos.start_column);
		}
		out = fmt::format_to(out, fmt, std::forward<T>(args)...);
		return { result.data(), result.size() };
	}

	struct empty_type {};

	template<typename T>
	struct conditional_target_type {
		using type = T::target_type;
	};

	template<>
	struct conditional_target_type<void> {
		using type = empty_type;
	};

	template<typename T>
	struct conditional_target_pointer_type {
		using type = T::target_type;
	};

	template<>
	struct conditional_target_pointer_type<void> {
		using type = empty_type;
	};
}
