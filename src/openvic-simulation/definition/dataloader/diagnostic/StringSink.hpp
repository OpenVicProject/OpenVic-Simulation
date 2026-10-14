#pragma once

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/TextSink.hpp"

namespace OpenVic::dataloader {
	class StringSink final : public TextSink {
	public:
		explicit StringSink(memory::string& out);
		~StringSink() override;

	private:
		memory::string& _out;

		bool can_render_style() const override;
		void render_to(memory_buffer_type& buf) override;
	};
}
