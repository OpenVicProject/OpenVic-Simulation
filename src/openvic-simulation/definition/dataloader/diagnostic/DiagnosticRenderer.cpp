#include "DiagnosticRenderer.hpp"

#include "openvic-simulation/core/memory/SmartPtr.hpp"
#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticSink.hpp"

using namespace OpenVic::dataloader;

void DiagnosticRenderer::render(DiagnosticItem const& item) const {
	for (memory::unique_base_ptr<DiagnosticSink> const& sink : _sinks) {
		sink->consume(item);
	}
}

void DiagnosticRenderer::render(forwardable_span<const DiagnosticItem> const& items) const {
	for (memory::unique_base_ptr<DiagnosticSink> const& sink : _sinks) {
		sink->begin();
	}

	for (DiagnosticItem const& item : items) {
		render(item);
	}

	for (memory::unique_base_ptr<DiagnosticSink> const& sink : _sinks) {
		sink->end();
		sink->flush();
	}
}

void DiagnosticRenderer::render(DiagnosticBag const& bag) const {
	render(bag.diagnostics());
}

void DiagnosticRenderer::flush() const {
	for (memory::unique_base_ptr<DiagnosticSink> const& sink : _sinks) {
		sink->flush();
	}
}

DiagnosticSink* DiagnosticRenderer::append_sink(sink_pointer_type sink) {
	return _sinks.emplace_back(std::move(sink)).get();
}

DiagnosticRenderer::sink_vector::const_iterator DiagnosticRenderer::erase_sink(sink_vector::const_iterator pos) {
	return _sinks.erase(pos);
}

void DiagnosticRenderer::clear_sinks() {
	_sinks.clear();
}

DiagnosticRenderer::sink_vector const& DiagnosticRenderer::get_sinks() const {
	return _sinks;
}
