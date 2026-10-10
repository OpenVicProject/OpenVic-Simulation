#include "TextSink.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>

#include <fmt/base.h>
#include <fmt/color.h>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticItem.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"

using namespace OpenVic::dataloader;

static TextSink::text_iterator find_code_point_boundary(TextSink::text_iterator cur, TextSink::text_iterator end) {
	// UTF-8 continuation code point unit
	while (cur != end && (*cur & 0b1100'0000) == (0b10 << 6)) {
		++cur;
	}
	return cur;
}

TextSink::~TextSink() = default;

void TextSink::consume(DiagnosticItem const& item) {
	open_file(item.filepath());

	memory_buffer_type buf;

	render_level(item, buf);
	render_text(buf, item.message());
	render_text(buf, '\n');

	render_filepath(item, buf);
	render_empty_annotation(buf);

	if (item.is_primary()) {
		for (DiagnosticItem const* subitem : item.related_items()) {
			render_annotation(*subitem, buf, "here");
			render_empty_annotation(buf);
			render_text(buf, '\n');
		}
	}

	render_annotation(item, buf, "here");

	render_to(buf);
}

void TextSink::end() {
	_file_content.clear();
	_file_content.shrink_to_fit();
}

void TextSink::render_text(memory_buffer_type& buf, std::string_view message) {
	buf.append(message);
}

void TextSink::render_text(memory_buffer_type& buf, char c) {
	buf.push_back(c);
}

void TextSink::render_style(memory_buffer_type& buf, fmt::text_style ts) {
	if (ts != fmt::text_style {} && can_render_style()) {
		if (ts.has_emphasis()) {
			fmt::detail::ansi_color_escape<char> emphasis = fmt::detail::make_emphasis<char>(ts.get_emphasis());
			buf.append(emphasis.begin(), emphasis.end());
		}
		if (ts.has_foreground()) {
			fmt::detail::ansi_color_escape<char> foreground = fmt::detail::make_foreground_color<char>(ts.get_foreground());
			buf.append(foreground.begin(), foreground.end());
		}
		if (ts.has_background()) {
			fmt::detail::ansi_color_escape<char> background = fmt::detail::make_background_color<char>(ts.get_background());
			buf.append(background.begin(), background.end());
		}
	}
}

void TextSink::render_style_reset(memory_buffer_type& buf) {
	if (can_render_style()) {
		fmt::detail::reset_color(buf);
	}
}

void TextSink::render_filepath(DiagnosticItem const& item, memory_buffer_type& buf) {
	if (item.filepath().empty() || _file_content.empty() || _file_content[0] == '\0') {
		return;
	}

	render_formatted_text(
	    buf,
	    fmt::emphasis::bold | fmt::fg(fmt::color::blue),
	    "{}:{}:{}\n",
	    item.filepath(),
	    item.location().start_line,
	    item.location().start_column
	);
}

void TextSink::render_empty_annotation(memory_buffer_type& buf) {
	render_formatted_text(buf, "     {}\n", get_column());
}

static size_t get_visible_width(
    TextSink::memory_buffer_type::value_type const* begin, TextSink::memory_buffer_type::value_type const* end
) {
	size_t result = 0;
	for (; begin != end; ++begin) {
		unsigned char value = static_cast<unsigned char>(*begin);
		if (value == '\e' && end - begin > 2) {
			if (begin[1] == '[') {
				begin += 2;
			} else {
				continue;
			}

			for (; begin != end; ++begin) {
				unsigned char c = static_cast<unsigned char>(*begin);
				if (c >= 0x40 && c <= 0x7E) {
					break;
				}
			}

			if (begin == end || ++begin == end) {
				break;
			}
			continue;
		}

		if ((value & 0b1100'0000) != 0b1000'0000) {
			++result;
		}
	}
	return result;
}

void TextSink::render_annotation(DiagnosticItem const& item, memory_buffer_type& buf, std::string_view annotation) {
	content_marker line_marker = find_in_content(item.location().start_line, 1);
	content_marker line_marker_end = find_in_content(item.location().start_line + 1, 1, line_marker);
	content_marker begin_marker = find_in_content(item.location().start_line, item.location().start_column, line_marker);
	content_marker end_marker = find_in_content(item.location().end_line, item.location().end_column - 1, begin_marker);

	text_iterator line_begin = line_marker.position;
	text_iterator begin = begin_marker.position;
	text_iterator end = end_marker.position;
	text_iterator newline_end = line_marker_end.position;

	bool truncated_multiline = false;
	if (begin == end) {
		if (end != newline_end) {
			// Empty range before end of newline; extend by one code unit.
			++end;
		}
	} else if (end > newline_end) {
		end = newline_end;
		truncated_multiline = true;
	}

	bool rounded_end;
	{
		text_iterator old_end = end;
		end = find_code_point_boundary(end, newline_end);
		rounded_end = end != old_end;
	}

	struct lexeme {
		text_iterator begin, end;
	};

	bool annotated_newline = end > newline_end;
	lexeme before = { line_begin, begin };
	lexeme annotated;
	lexeme after;
	if (!annotated_newline) {
		annotated = { begin, end };
		after = { end, newline_end };
	} else {
		annotated = { begin, newline_end };
		after = { newline_end, newline_end };
	}

	bool annotate_eof = truncated_multiline && !annotated_newline;

	render_formatted_text(buf, fmt::fg(fmt::color::blue), "{:4} ", item.location().start_line);
	render_formatted_text(buf, "{} ", get_column());

	size_t indent_before;
	{
		size_t count_start = buf.size();
		render_unicode_display(buf, before.begin, before.end);
		memory_buffer_type::value_type const* count_end = buf.end();
		indent_before = get_visible_width(buf.data() + count_start, count_end);
	}

	if (item.level() <= DiagnosticLevel::ERROR) {
		render_style(buf, get_style_for(item) | fmt::emphasis::bold);
	} else {
		render_style(buf, get_style_for(item));
	}
	size_t underline_count;
	{

		size_t count_start = buf.size();
		render_unicode_display(buf, annotated.begin, annotated.end);
		memory_buffer_type::value_type const* count_end = buf.end();
		underline_count = get_visible_width(buf.data() + count_start, count_end);
	}
	render_style_reset(buf);

	render_unicode_display(buf, after.begin, after.end);
	render_text(buf, '\n');

	render_formatted_text(buf, "     {} ", get_column());
	{
		size_t indent_start = buf.size();
		buf.resize(buf.size() + indent_before);
		std::memset(buf.data() + indent_start, ' ', indent_before);
	}

	if (item.level() <= DiagnosticLevel::ERROR) {
		render_style(buf, get_style_for(item) | fmt::emphasis::bold);
	} else {
		render_style(buf, get_style_for(item));
	}
	buf.reserve(buf.size() + underline_count + 3 + item.message().size());
	std::string_view underline = get_underline_for(item);
	for (size_t i = 0; i != underline_count; i += underline.size()) {
		render_text(buf, underline);
	}
	if (underline_count == 0 || annotate_eof) {
		render_text(buf, underline);
	}
	render_text(buf, ' ');
	render_text(buf, annotation);
	render_text(buf, '\n');
	render_style_reset(buf);
}

void TextSink::render_level(DiagnosticItem const& item, memory_buffer_type& buf) {
	switch (item.level()) {
	case DiagnosticLevel::NONE:    break;
	case DiagnosticLevel::FATAL:   render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "fatal: "); break;
	case DiagnosticLevel::ERROR:   render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "error: "); break;
	case DiagnosticLevel::WARN:    render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "warning: "); break;
	case DiagnosticLevel::INFO:    render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "info: "); break;
	case DiagnosticLevel::HINT:    render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "hint: "); break;
	case DiagnosticLevel::SUGGEST: render_formatted_text(buf, fmt::emphasis::bold | get_style_for(item), "suggest: "); break;
	}
}

struct code_point_result {
	char32_t code_point_value = 0;
	enum class error_t {
		SUCCESS,
		END_OF_FILE,
		LEADS_WITH_TRAILING,
		MISSING_TRAILING,
		SURROGATE,
		OVERLONG_SEQUENCE,
		OUT_OF_RANGE,
	} error {};
	TextSink::text_iterator end;
};

static code_point_result parse_code_point(TextSink::text_iterator begin, TextSink::text_iterator end) {
	using uchar_t = unsigned char;
	constexpr auto payload_lead1 = 0b0111'1111;
	constexpr auto payload_lead2 = 0b0001'1111;
	constexpr auto payload_lead3 = 0b0000'1111;
	constexpr auto payload_lead4 = 0b0000'0111;
	constexpr auto payload_cont = 0b0011'1111;

	constexpr auto pattern_lead1 = 0b0 << 7;
	constexpr auto pattern_lead2 = 0b110 << 5;
	constexpr auto pattern_lead3 = 0b1110 << 4;
	constexpr auto pattern_lead4 = 0b11110 << 3;
	constexpr auto pattern_cont = 0b10 << 6;

	if (begin == end) {
		return { {}, code_point_result::error_t::END_OF_FILE, begin };
	}

	auto first = uchar_t(*begin);
	if ((first & ~payload_lead1) == pattern_lead1) {
		// ASCII character.
		++begin;
		return { first, code_point_result::error_t::SUCCESS, begin };
	} else if ((first & ~payload_cont) == pattern_cont) {
		return { {}, code_point_result::error_t::LEADS_WITH_TRAILING, begin };
	} else if ((first & ~payload_lead2) == pattern_lead2) {
		++begin;

		auto second = uchar_t(*begin);
		if ((second & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto result = char32_t(first & payload_lead2);
		result <<= 6;
		result |= char32_t(second & payload_cont);

		// C0 and C1 are overlong ASCII.
		if (first == 0xC0 || first == 0xC1) {
			return { result, code_point_result::error_t::OVERLONG_SEQUENCE, begin };
		} else {
			return { result, code_point_result::error_t::SUCCESS, begin };
		}
	} else if ((first & ~payload_lead3) == pattern_lead3) {
		++begin;

		auto second = uchar_t(*begin);
		if ((second & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto third = uchar_t(*begin);
		if ((third & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto result = char32_t(first & payload_lead3);
		result <<= 6;
		result |= char32_t(second & payload_cont);
		result <<= 6;
		result |= char32_t(third & payload_cont);

		auto cp = result;
		if (0xD800 <= cp && cp <= 0xDFFF) {
			return { cp, code_point_result::error_t::SURROGATE, begin };
		} else if (first == 0xE0 && second < 0xA0) {
			return { cp, code_point_result::error_t::OVERLONG_SEQUENCE, begin };
		} else {
			return { cp, code_point_result::error_t::SUCCESS, begin };
		}
	} else if ((first & ~payload_lead4) == pattern_lead4) {
		++begin;

		auto second = uchar_t(*begin);
		if ((second & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto third = uchar_t(*begin);
		if ((third & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto fourth = uchar_t(*begin);
		if ((fourth & ~payload_cont) != pattern_cont) {
			return { {}, code_point_result::error_t::MISSING_TRAILING, begin };
		}
		++begin;

		auto result = char32_t(first & payload_lead4);
		result <<= 6;
		result |= char32_t(second & payload_cont);
		result <<= 6;
		result |= char32_t(third & payload_cont);
		result <<= 6;
		result |= char32_t(fourth & payload_cont);

		auto cp = result;
		if (cp > 0x10'FFFF) {
			return { cp, code_point_result::error_t::OUT_OF_RANGE, begin };
		} else if (first == 0xF0 && second < 0x90) {
			return { cp, code_point_result::error_t::OVERLONG_SEQUENCE, begin };
		} else {
			return { cp, code_point_result::error_t::SUCCESS, begin };
		}
	} else // FE or FF
	{
		return { {}, code_point_result::error_t::END_OF_FILE, begin };
	}
}

static TextSink::text_iterator get_recovered_code_point(
    TextSink::text_iterator begin, TextSink::text_iterator end, code_point_result result
) {
	switch (result.error) {
		using enum code_point_result::error_t;
	case SUCCESS:
		// Consume the entire code point.
		return result.end;
	case END_OF_FILE: return begin;

	case LEADS_WITH_TRAILING:
		// Invalid code unit, consume to recover.
		assert(result.end == begin);
		return begin + 1;

	case MISSING_TRAILING:
	case SURROGATE:
	case OUT_OF_RANGE:
	case OVERLONG_SEQUENCE:
		// Consume all the invalid code units to recover.
		return result.end;
	}
	OpenVic::unreachable();
}

void TextSink::render_unicode_display(memory_buffer_type& buf, text_iterator begin, text_iterator end) {
	size_t count = 0;
	while (true) {
		if (code_point_result result = parse_code_point(begin, end); result.error == code_point_result::error_t::END_OF_FILE) {
			break;
		} else if (result.error == code_point_result::error_t::SUCCESS) {
			begin = result.end;
			render_code_point(buf, result.code_point_value);
		} else {
			// Recover from the failed code point.
			text_iterator cur = begin;
			begin = get_recovered_code_point(begin, end, result);

			// Visualize each skipped code unit as byte.
			for (; cur != begin; ++cur) {
				render_formatted_text(buf, "0x{:02X}", static_cast<unsigned char>(*cur & 0xFF));
			}
		}

		++count;
	}
}

void TextSink::render_code_point(memory_buffer_type& buf, char32_t code_point) {
	using namespace std::string_view_literals;

	// If code_point is invalid
	if (code_point > 0x10'FFFF) {
		return buf.append("U+????"sv);
	}

	// If code_point is control character
	if (code_point <= 0x1F || (0x7F <= code_point && code_point <= 0x9F)) {
		int v = code_point;
		switch (v) {
		case '\0': return render_text(buf, "⟨NUL⟩"sv);
		case '\r': return render_text(buf, "⟨CR⟩"sv);
		case '\n': return render_formatted_text(buf, fmt::emphasis::faint, "{}", "⏎");
		case '\t': return render_formatted_text(buf, fmt::emphasis::faint, "{}", "⇨");
		default:   break;
		}
	}

	if (code_point == ' ') {
		return render_text(buf, ' ');
	}

	if (code_point <= 0x7F) {
		return render_text(buf, static_cast<char>(code_point));
	}

	return render_formatted_text(buf, "⟨U+{:04X}⟩", static_cast<int>(code_point));
}

std::string_view TextSink::get_column() const {
	return reinterpret_cast<char const*>(u8"│");
}

std::string_view TextSink::get_underline_for(DiagnosticItem const& item) const {
	if (item.is_secondary()) {
		return "~";
	}
	return "^";
}

fmt::text_style TextSink::get_style_for(DiagnosticItem const& item) const {
	switch (item.level()) {
	case DiagnosticLevel::NONE:    return {};
	case DiagnosticLevel::FATAL:   return fmt::fg(fmt::terminal_color::bright_red);
	case DiagnosticLevel::ERROR:   return fmt::fg(fmt::color::red);
	case DiagnosticLevel::WARN:    return fmt::fg(fmt::color::yellow);
	case DiagnosticLevel::INFO:    return fmt::fg(fmt::terminal_color::cyan);
	case DiagnosticLevel::HINT:    return fmt::fg(fmt::terminal_color::bright_blue);
	case DiagnosticLevel::SUGGEST: return fmt::fg(fmt::terminal_color::green);
	}
	OpenVic::unreachable();
}

bool TextSink::open_file(std::string_view filepath) {
	if (_file_content.size() == 1 && _file_content[0] == '\0') {
		return false;
	}

	if (!_file_content.empty()) {
		return true;
	}

	uintmax_t size = std::filesystem::file_size(filepath);
	_file_content.resize(static_cast<size_t>(size));

	std::FILE* file;
	{
		memory::string path { filepath };
		file = std::fopen(path.c_str(), "r+");
		if (!file) {
			_file_content.resize(1);
			return false;
		}
	}

	size_t read = std::fread(_file_content.data(), sizeof(memory::string::value_type), size, file);
	std::fclose(file);
	if (read != _file_content.size()) {
		_file_content.resize(1);
		return false;
	}

	return true;
}

TextSink::content_marker TextSink::find_in_content(size_t line, size_t column, TextSink::content_marker marker) {
	if (marker.line > line || marker.line == line && marker.column > column) {
		return marker;
	}

	if (line >= _file_content.size() || column >= _file_content.size()) {
		return { 0, 0, marker.position };
	}

	if (marker.position == text_iterator {}) {
		marker = { 1, 1, _file_content.begin() };
	}

	char const* start_ptr = nullptr;
	for (size_t column_count = marker.line == line ? marker.column : 1;
	     marker.line <= line && marker.position != _file_content.end();
	     ++marker.position) {
		char const& c = *marker.position;
		if (c == '\n') {
			++marker.line;
			continue;
		}

		if (marker.line != line) {
			continue;
		}

		if (c == '\r' && marker.position + 1 != _file_content.end() && marker.position[1] == '\n') {
			continue;
		}

		if (column_count == column) {
			start_ptr = &c;
			break;
		}
		++column_count;
	}

	if (start_ptr == nullptr) {
		return { 0, 0, marker.position };
	}

	return { line, column, _file_content.begin() + (start_ptr - _file_content.data()) };
}
