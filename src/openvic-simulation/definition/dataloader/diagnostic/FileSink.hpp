#pragma once

#include <cstdio>

#include "openvic-simulation/definition/dataloader/diagnostic/TextSink.hpp"

namespace OpenVic::dataloader {
	class FileSink final : public TextSink {
	public:
		struct Options {
			bool use_styles = true;
		};

		explicit FileSink(std::FILE* file, Options opts = { .use_styles = true });

	private:
		std::FILE* _file;
		Options _options;

		bool can_render_style() const override;
		void render_to(memory_buffer_type& buf) override;
	};
}
