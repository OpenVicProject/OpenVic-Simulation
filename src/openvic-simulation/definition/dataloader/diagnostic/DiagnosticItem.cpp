#include "DiagnosticItem.hpp"

#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticCode.hpp"

using namespace OpenVic::dataloader;

DiagnosticItem::DiagnosticItem(DiagnosticBag& bag, node_pointer_type node) :
    DiagnosticItem { bag, bag.parser() != nullptr ? bag.parser()->get_position(node) : ovdl::FilePosition {} } {}

DiagnosticItem::DiagnosticItem(DiagnosticBag& bag, ovdl::FilePosition location) : _bag { &bag }, _location(location) {}

DiagnosticItem& DiagnosticItem::with_level(DiagnosticLevel level) {
	_level = level;
	return *this;
}

DiagnosticItem& DiagnosticItem::with_code(diagnostic_code_t diagnostic_code) {
	_diagnostic_code = diagnostic_code;
	return *this;
}

DiagnosticItem& DiagnosticItem::with_code(type_safe::underlying_type<diagnostic_code_t> diagnostic_code) {
	return with_code(diagnostic_code_t { diagnostic_code });
}

DiagnosticItem& DiagnosticItem::info(node_pointer_type related_node) {
	return item(DiagnosticLevel::INFO, related_node);
}

DiagnosticItem& DiagnosticItem::info(ovdl::FilePosition related_location) {
	return item(DiagnosticLevel::INFO, related_location);
}

DiagnosticItem& DiagnosticItem::hint(node_pointer_type related_node) {
	return item(DiagnosticLevel::HINT, related_node);
}

DiagnosticItem& DiagnosticItem::hint(ovdl::FilePosition related_location) {
	return item(DiagnosticLevel::HINT, related_location);
}

DiagnosticItem& DiagnosticItem::suggestion(node_pointer_type related_node) {
	return item(DiagnosticLevel::SUGGEST, related_node);
}

DiagnosticItem& DiagnosticItem::suggestion(ovdl::FilePosition related_location) {
	return item(DiagnosticLevel::SUGGEST, related_location);
}

DiagnosticItem& DiagnosticItem::item(DiagnosticLevel level, node_pointer_type related_node) {
	return item(level, _bag->parser() != nullptr ? _bag->parser()->get_position(related_node) : ovdl::FilePosition {});
}

DiagnosticItem& DiagnosticItem::item(DiagnosticLevel level, ovdl::FilePosition related_location) {
	DiagnosticItem& reported =
	    _bag->report({ *_bag, related_location.is_empty() ? _location : related_location }).with_level(level);
	reported._primary = false;
	_related_items.emplace_back(&reported);
	reported._related_items.emplace_back(this);
	return reported;
}

std::string_view DiagnosticItem::message() const& {
	return _message;
}

OpenVic::memory::string DiagnosticItem::message() && {
	return OV_MOV(_message);
}

std::string_view DiagnosticItem::filepath() const {
	return _bag->parser() != nullptr ? _bag->parser()->get_file_path() : std::string_view {};
}

OpenVic::memory::vector<DiagnosticItem const*> const& DiagnosticItem::related_items() const {
	return _related_items;
}

ovdl::FilePosition DiagnosticItem::location() const {
	return _location;
}

DiagnosticLevel DiagnosticItem::level() const {
	return _level;
}

bool DiagnosticItem::is_primary() const {
	return _primary;
}

bool DiagnosticItem::is_secondary() const {
	return !_primary;
}

diagnostic_code_t DiagnosticItem::diagnostic_code() const {
	return _diagnostic_code;
}

DiagnosticBag& DiagnosticItem::diagnostic() {
	return *_bag;
}

DiagnosticBag const& DiagnosticItem::diagnostic() const {
	return *_bag;
}
