#pragma once

#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <fmt/base.h>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/Formatting.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticCode.hpp"
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

		DiagnosticItem& with_code(diagnostic_code_t diagnostic_code);
		DiagnosticItem& with_code(type_safe::underlying_type<diagnostic_code_t> diagnostic_code);

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

		std::string_view message() const & OV_LIFETIME_BOUND;
		memory::string message() &&;

		std::string_view filepath() const;

		memory::vector<DiagnosticItem const*> const& related_items() const OV_LIFETIME_BOUND;

		ovdl::FilePosition location() const;
		DiagnosticLevel level() const;
		bool is_primary() const;
		bool is_secondary() const;
		diagnostic_code_t diagnostic_code() const;

		DiagnosticBag& diagnostic() OV_LIFETIME_BOUND;
		DiagnosticBag const& diagnostic() const OV_LIFETIME_BOUND;

	private:
		memory::string _message;
		memory::vector<DiagnosticItem const*> _related_items;
		ovdl::FilePosition _location;
		DiagnosticBag* _bag = nullptr;
		diagnostic_code_t _diagnostic_code { 0u };
		bool _primary = true;
		DiagnosticLevel _level = DiagnosticLevel::NONE;
	};
}
