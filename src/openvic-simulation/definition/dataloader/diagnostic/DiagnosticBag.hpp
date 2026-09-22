#pragma once

#include <utility>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <range/v3/view/filter.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticItem.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

namespace spdlog {
	class logger;
}

namespace OpenVic::dataloader {
	class DiagnosticBag {
	public:
		friend class DiagnosticItem;

		DiagnosticBag(ovdl::v2script::Parser const* parser);

		DiagnosticItem& fatal(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& error(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& warning(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& info(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& hint(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& suggestion(ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;
		DiagnosticItem& report(DiagnosticLevel level, ovdl::v2script::ast::Node const* node) OV_LIFETIME_BOUND;

		void clear();
		void sort();

		DiagnosticBag as_sorted() const;

		void log_to(spdlog::logger* logger) const;

		ovdl::v2script::Parser const* parser() const OV_LIFETIME_BOUND;
		memory::vector<DiagnosticItem> const& diagnostics() const OV_LIFETIME_BOUND;

		size_t error_count() const;
		size_t warning_count() const;
		bool has_fatal_error() const;

		template<typename Predicate>
		auto filter(Predicate&& predicate) const OV_LIFETIME_BOUND {
			return _diagnostics | ranges::views::filter(std::forward<Predicate>(predicate));
		}

		auto errors() const OV_LIFETIME_BOUND {
			return filter([](DiagnosticItem const& d) {
				return d.level() <= DiagnosticLevel::ERROR;
			});
		}

		auto warnings() const OV_LIFETIME_BOUND {
			return filter([](DiagnosticItem const& d) {
				return d.level() == DiagnosticLevel::WARN;
			});
		}

		auto of_level(DiagnosticLevel level) const OV_LIFETIME_BOUND {
			return filter([level](DiagnosticItem const& d) {
				return d.level() == level;
			});
		}

	private:
		memory::vector<DiagnosticItem> _diagnostics;
		ovdl::v2script::Parser const* _parser;
		size_t _error_count = 0;
		size_t _warning_count = 0;
		bool _fatal_error = false;

		DiagnosticItem& report(DiagnosticItem&& item) OV_LIFETIME_BOUND;

		template<bool IsParserNullptr>
		void log_to_impl(spdlog::logger* logger) const;
	};
}
