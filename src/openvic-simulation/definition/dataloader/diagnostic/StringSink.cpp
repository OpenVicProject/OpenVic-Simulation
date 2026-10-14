#include "StringSink.hpp"

#include "openvic-simulation/core/memory/String.hpp"

using namespace OpenVic::dataloader;

StringSink::StringSink(memory::string& out) : _out { out } {}

StringSink::~StringSink() = default;

bool StringSink::can_render_style() const {
	return false;
}

void StringSink::render_to(memory_buffer_type& buf) {
	_out = { buf.data(), buf.size() };
}
