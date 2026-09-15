#pragma once

#include <string_view>
#include <type_traits>

#include <openvic-dataloader/NodeLocation.hpp>

#include <fmt/base.h>
#include <fmt/color.h>

#include "openvic-simulation/core/memory/Formatting.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticSink.hpp"

namespace OpenVic::dataloader {
	class DiagnosticItem;

	class TextSink : public DiagnosticSink {
	public:
		using memory_buffer_type = memory::fmt::basic_memory_buffer<char, 250>;
		using text_iterator = memory::string::const_iterator;

		// enum class Color {
		// 	RESET = 0,
		// 	BOLD = 1,
		// 	FAINT = 2,
		// 	ITALIC = 3,

		// 	BLACK = 30,
		// 	RED = 31,
		// 	GREEN = 32,
		// 	YELLOW = 33,
		// 	BLUE = 34,
		// 	MAGENTA = 35,
		// 	CYAN = 36,
		// 	WHITE = 37,
		// };

		void consume(DiagnosticItem const& item) override;

		void end() override;

		~TextSink() override;

	protected:
		virtual bool can_render_style() const = 0;
		virtual void render_to(memory_buffer_type& buf) = 0;

		void render_text(memory_buffer_type& buf, std::string_view message);
		void render_text(memory_buffer_type& buf, char c);

		void render_unicode_display(memory_buffer_type& buf, text_iterator begin, text_iterator end);

		void render_style(memory_buffer_type& buf, fmt::text_style ts);
		void render_style_reset(memory_buffer_type& buf);

		void render_filepath(DiagnosticItem const& item, memory_buffer_type& buf);

		void render_empty_annotation(memory_buffer_type& buf);

		void render_annotation(DiagnosticItem const& item, memory_buffer_type& buf, std::string_view annotation);

		void render_level(DiagnosticItem const& item, memory_buffer_type& buf);

		void render_code_point(memory_buffer_type& buf, char32_t code_point);

		std::string_view get_column() const;
		std::string_view get_underline_for(DiagnosticItem const& item) const;
		fmt::text_style get_style_for(DiagnosticItem const& item) const;

		template<typename... Args>
		void render_formatted_text(memory_buffer_type& buf, fmt::format_string<Args...> fmt, Args&&... args) {
			if (!can_render_style()) {
				memory_buffer_type fmt_buf;
				auto out = fmt::appender(fmt_buf);
				fmt::vformat_to(out, fmt, fmt::make_format_args(remove_style(args)...));
				render_text(buf, std::string_view { fmt_buf.data(), fmt_buf.size() });
				return;
			}

			memory_buffer_type fmt_buf;
			auto out = fmt::appender(fmt_buf);
			fmt::vformat_to(out, fmt, fmt::make_format_args(args...));
			render_text(buf, std::string_view { fmt_buf.data(), fmt_buf.size() });
		}

		template<typename... Args>
		void render_formatted_text(
		    memory_buffer_type& buf, fmt::text_style ts, fmt::format_string<Args...> fmt, Args&&... args
		) {
			if (!can_render_style()) {
				memory_buffer_type fmt_buf;
				auto out = fmt::appender(fmt_buf);
				fmt::vformat_to(out, fmt, fmt::make_format_args(remove_style(args)...));
				render_text(buf, std::string_view { fmt_buf.data(), fmt_buf.size() });
				return;
			}

			memory_buffer_type fmt_buf;
			auto out = fmt::appender(fmt_buf);
			fmt::vformat_to(out, ts, fmt, fmt::make_format_args(args...));
			render_text(buf, std::string_view { fmt_buf.data(), fmt_buf.size() });
		}

	private:
		memory::string _file_content;

		struct content_marker {
			size_t line = 0;
			size_t column = 0;
			text_iterator position;

			constexpr bool is_invalid() const {
				return line == 0 && column == 0;
			}
		};

		bool open_file(std::string_view filepath);
		content_marker find_in_content(size_t line, size_t column, content_marker begin = { 0, 0 });

		template<typename T>
		static constexpr decltype(auto) remove_style(T&& value) {
			using ref_type = std::remove_cvref_t<T>;
			if constexpr (specialization_of<ref_type, fmt::detail::styled_arg>) {
				return value.value;
			} else {
				return value;
			}
		}
	};
}
