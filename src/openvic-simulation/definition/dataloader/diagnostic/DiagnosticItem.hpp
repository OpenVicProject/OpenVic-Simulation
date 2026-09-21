#pragma once

#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <fmt/base.h>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/Formatting.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

namespace OpenVic::dataloader {
	class DiagnosticBag;

	class DiagnosticItem {
	public:
		friend class DiagnosticBag;

		using node_pointer_type = ovdl::v2script::ast::Node const*;

		DiagnosticItem(DiagnosticBag& bag, node_pointer_type node);
		DiagnosticItem(DiagnosticBag& bag, ovdl::FilePosition location);

		DiagnosticItem& with_level(DiagnosticLevel level);

		template<typename... T>
		DiagnosticItem& with_message(fmt::format_string<T...> fmt, T&&... args) {
			memory::fmt::basic_memory_buffer<char, 250> result {};
			auto out = fmt::appender(result);
			out = fmt::format_to(out, fmt, std::forward<T>(args)...);
			_message = { result.data(), result.size() };
			return *this;
		}

		DiagnosticItem& info(node_pointer_type related_node = nullptr);
		DiagnosticItem& info(ovdl::FilePosition related_location);

		DiagnosticItem& hint(node_pointer_type related_node = nullptr);
		DiagnosticItem& hint(ovdl::FilePosition related_location);

		DiagnosticItem& suggestion(node_pointer_type related_node = nullptr);
		DiagnosticItem& suggestion(ovdl::FilePosition related_location);

		DiagnosticItem& item(DiagnosticLevel level, node_pointer_type related_node = nullptr);
		DiagnosticItem& item(DiagnosticLevel level, ovdl::FilePosition related_location);

		std::string_view message() const OV_LIFETIME_BOUND;
		std::string_view filepath() const OV_LIFETIME_BOUND;
		memory::vector<DiagnosticItem const*> const& related_items() const OV_LIFETIME_BOUND;
		ovdl::FilePosition location() const;
		DiagnosticLevel level() const;
		bool is_primary() const;
		bool is_secondary() const;

		DiagnosticBag& diagnostic() OV_LIFETIME_BOUND;
		DiagnosticBag const& diagnostic() const OV_LIFETIME_BOUND;

	private:
		DiagnosticBag* _bag;
		memory::string _message;
		memory::vector<DiagnosticItem const*> _related_items;
		ovdl::FilePosition _location;
		DiagnosticLevel _level = DiagnosticLevel::NONE;
		bool _primary = true;
	};
}
