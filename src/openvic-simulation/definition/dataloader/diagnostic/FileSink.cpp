#include "FileSink.hpp"

#include <fmt/base.h>

using namespace OpenVic::dataloader;

FileSink::FileSink(std::FILE* file, Options opts) : _file { file }, _options { opts } {}

bool FileSink::can_render_style() const {
	return _options.use_styles;
}

void FileSink::render_to(memory_buffer_type& buf) {
	fmt::print(_file, "{}", fmt::string_view { buf.data(), buf.size() });
}
