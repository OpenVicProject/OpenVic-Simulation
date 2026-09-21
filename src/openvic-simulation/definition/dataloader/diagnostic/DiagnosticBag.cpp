#include "DiagnosticBag.hpp"

#include <fmt/base.h>

#include <range/v3/algorithm/sort.hpp>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "openvic-simulation/core/memory/Formatting.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticItem.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

using namespace OpenVic::dataloader;

DiagnosticBag::DiagnosticBag(ovdl::v2script::Parser const* parser) : _parser(parser) {}

DiagnosticItem& DiagnosticBag::fatal(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::FATAL, node);
}

DiagnosticItem& DiagnosticBag::error(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::ERROR, node);
}

DiagnosticItem& DiagnosticBag::warning(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::WARN, node);
}

DiagnosticItem& DiagnosticBag::info(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::INFO, node);
}

DiagnosticItem& DiagnosticBag::hint(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::HINT, node);
}

DiagnosticItem& DiagnosticBag::suggestion(ovdl::v2script::ast::Node const* node) {
	return report(DiagnosticLevel::SUGGEST, node);
}

DiagnosticItem& DiagnosticBag::report(DiagnosticLevel level, ovdl::v2script::ast::Node const* node) {
	return report([&] {
		DiagnosticItem item(*this, node);
		item.with_level(level);
		return item;
	}());
}

DiagnosticItem& DiagnosticBag::report(DiagnosticItem&& item) {
	DiagnosticItem& report = _diagnostics.emplace_back(std::move(item));
	report._bag = this;
	switch (report.level()) {
	case DiagnosticLevel::FATAL: _fatal_error = true; [[fallthrough]];
	case DiagnosticLevel::ERROR: _error_count++; break;
	case DiagnosticLevel::WARN:  _warning_count++; break;
	default:                     break;
	}
	return report;
}

void DiagnosticBag::clear() {
	_diagnostics.clear();
}

void DiagnosticBag::sort() {
	if (parser() == nullptr) {
		ranges::sort(_diagnostics, [](DiagnosticItem const& a, DiagnosticItem const& b) -> bool {
			if (a.is_primary() != b.is_primary()) {
				return a.is_primary();
			}

			if (a.level() != b.level() && a.level() > DiagnosticLevel::WARN && b.level() > DiagnosticLevel::WARN) {
				return a.level() > b.level();
			}

			return true;
		});
	}

	ranges::sort(_diagnostics, [](DiagnosticItem const& a, DiagnosticItem const& b) -> bool {
		if (a.is_primary() != b.is_primary()) {
			return a.is_primary();
		}

		if (a.level() != b.level() && a.level() > DiagnosticLevel::WARN && b.level() > DiagnosticLevel::WARN) {
			return a.level() > b.level();
		}

		if (a.location().start_line != b.location().start_line) {
			return a.location().start_line < b.location().start_line;
		}

		return a.location().start_column < b.location().start_column;
	});
}

DiagnosticBag DiagnosticBag::as_sorted() const {
	DiagnosticBag sorted(*this);
	sorted.sort();
	return sorted;
}

template<bool IsParserNullptr>
void DiagnosticBag::log_to_impl(spdlog::logger* logger) const {
	for (DiagnosticItem const& item : _diagnostics) {
		if (item.is_secondary()) {
			continue;
		}

		spdlog::level::level_enum log_level;
		switch (item.level()) {
		case DiagnosticLevel::NONE:    continue;
		case DiagnosticLevel::FATAL:
		case DiagnosticLevel::ERROR:   log_level = spdlog::level::err; break;
		case DiagnosticLevel::WARN:    log_level = spdlog::level::warn; break;
		case DiagnosticLevel::INFO:
		case DiagnosticLevel::HINT:
		case DiagnosticLevel::SUGGEST: log_level = spdlog::level::info; break;
		}

		if (item.related_items().empty()) {
			if constexpr (IsParserNullptr) {
				logger->log(log_level, item.message());
			} else {
				logger->log(
				    log_level,
				    "{}:{}:{}: {}",
				    item.filepath(),
				    item.location().start_line,
				    item.location().start_column,
				    item.message()
				);
			}
		} else {
			memory::fmt::basic_memory_buffer<char, 250> buf;
			auto out = fmt::appender(buf);

			DiagnosticItem const* last = item.related_items().back();
			for (DiagnosticItem const* sec : item.related_items()) {
				switch (sec->level()) {
				case DiagnosticLevel::NONE:    break;
				case DiagnosticLevel::FATAL:   out = fmt::detail::write<char>(out, "fatal: "); break;
				case DiagnosticLevel::ERROR:   out = fmt::detail::write<char>(out, "error: "); break;
				case DiagnosticLevel::WARN:    out = fmt::detail::write<char>(out, "warning: "); break;
				case DiagnosticLevel::INFO:    out = fmt::detail::write<char>(out, "info: "); break;
				case DiagnosticLevel::HINT:    out = fmt::detail::write<char>(out, "hint: "); break;
				case DiagnosticLevel::SUGGEST: out = fmt::detail::write<char>(out, "suggestion: "); break;
				}
				if constexpr (IsParserNullptr) {
					out = fmt::detail::write<char>(out, sec->message());
				} else {
					out = fmt::detail::write<char>(out, sec->filepath());
					out = fmt::detail::write<char>(out, ':');
					out = fmt::detail::write<char>(out, sec->filepath());
					out = fmt::detail::write<char>(out, ':');
					out = fmt::detail::write<char>(out, sec->filepath());
					out = fmt::detail::write<char>(out, ": ");
					out = fmt::detail::write<char>(out, sec->message());
				}
				if (last != sec) {
					out = fmt::detail::write<char>(out, "\n\t");
				}
			}

			std::string_view sv { buf.data(), buf.size() };
			if constexpr (IsParserNullptr) {
				logger->log(log_level, "{}\n\t{}", item.message(), sv);
			} else {
				logger->log(
				    log_level,
				    "{}:{}:{}: {}\n\t{}",
				    item.filepath(),
				    item.location().start_line,
				    item.location().start_column,
				    item.message(),
				    sv
				);
			}
		}
	}
}

void DiagnosticBag::log_to(spdlog::logger* logger) const {
	if (parser() == nullptr) {
		log_to_impl<true>(logger);
		return;
	}

	log_to_impl<false>(logger);
}

ovdl::v2script::Parser const* DiagnosticBag::parser() const {
	return _parser;
}

OpenVic::memory::vector<DiagnosticItem> const& DiagnosticBag::diagnostics() const {
	return _diagnostics;
}

size_t DiagnosticBag::error_count() const {
	return _error_count;
}

size_t DiagnosticBag::warning_count() const {
	return _warning_count;
}

bool DiagnosticBag::has_fatal_error() const {
	return _fatal_error;
}
