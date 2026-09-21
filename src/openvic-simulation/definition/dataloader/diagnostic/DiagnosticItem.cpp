#include "DiagnosticItem.hpp"

#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"

using namespace OpenVic::dataloader;

DiagnosticItem::DiagnosticItem(DiagnosticBag& bag, node_pointer_type node) :
    DiagnosticItem { bag, bag.parser() != nullptr ? bag.parser()->get_position(node) : ovdl::FilePosition {} } {}

DiagnosticItem::DiagnosticItem(DiagnosticBag& bag, ovdl::FilePosition location) : _bag { &bag }, _location(location) {}

DiagnosticItem& DiagnosticItem::with_level(DiagnosticLevel level) {
	_level = level;
	return *this;
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
	DiagnosticItem& reported = _bag->report([&] {
		DiagnosticItem related(*_bag, related_location.is_empty() ? _location : related_location);
		related.with_level(level)._primary = false;
		return related;
	}());
	_related_items.emplace_back(&reported);
	reported._related_items.emplace_back(this);
	return reported;
}

std::string_view DiagnosticItem::message() const {
	return _message;
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

DiagnosticBag& DiagnosticItem::diagnostic() {
	return *_bag;
}

DiagnosticBag const& DiagnosticItem::diagnostic() const {
	return *_bag;
}
