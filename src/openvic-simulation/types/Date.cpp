#include "Date.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <limits>
#include <ostream>
#include <string_view>

#include <fmt/format.h>

#include <range/v3/algorithm/max_element.hpp>

#include "openvic-simulation/core/Typedefs.hpp"

using namespace OpenVic;

memory::string Timespan::to_string() const {
	// Maximum number of digits + 1 for potential minus sign
	static constexpr size_t array_length = fmt::detail::count_digits(uint64_t(std::numeric_limits<day_t>::max())) + 1;

	std::array<char, array_length> str_array {};
	std::to_chars_result result = to_chars(str_array.data(), str_array.data() + str_array.size());
	if (result.ec != std::errc {}) {
		return {};
	}

	return { str_array.data(), result.ptr };
}

Timespan::operator memory::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Timespan const& timespan) {
	return out << timespan.to_string();
}

memory::string Date::to_string(bool pad_year, bool pad_month, bool pad_day) const {
	stack_string result = to_array(pad_year, pad_month, pad_day);
	if (OV_unlikely(result.empty())) {
		return {};
	}

	return result;
}

Date::operator memory::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date date) {
	Date::stack_string result = date.to_array(false, false, false);
	if (OV_unlikely(result.empty())) {
		return out;
	}

	return out << static_cast<std::string_view>(result);
}

using namespace ovfmt::detail;

template<typename OutputIt, typename Char = char>
struct date_writer {
	bool _unsupported = false;

	inline static constexpr Date::day_t DAYS_PER_WEEK = Date::WEEKDAY_NAMES.size();

	constexpr date_writer(OutputIt out, Date const& date, Char = {}) : _out(out), _date(date) {}

	constexpr OutputIt out() const {
		return _out;
	}

	inline void unsupported() {
		_unsupported = true;
		report_error("no format");
	}

	constexpr void on_text(const Char* begin, const Char* end) {
		_out = fmt::detail::copy<Char>(begin, end, _out);
	}

	void on_year(numeric_system ns, pad_type pad) {
		return write_year(_date.get_year(), pad);
	}

	void on_short_year(numeric_system ns) {
		return write2(split_year_lower(_date.get_year()));
	}

	void on_century(numeric_system ns) {
		OpenVic::Date::year_t year = _date.get_year();
		OpenVic::Date::year_t upper = year / 100;
		if (year >= -99 && year < 0) {
			// Zero upper on negative year.
			*_out++ = '-';
			*_out++ = '0';
		} else if (upper >= 0 && upper < 100) {
			write2(static_cast<int>(upper));
		} else {
			_out = detail::write<Char>(_out, upper);
		}
	}

	void on_iso_week_based_year() {
		write_year(date_iso_week_year(), pad_type::zero);
	}

	void on_iso_week_based_short_year() {
		write2(split_year_lower(date_iso_week_year()));
	}

	void on_offset_year() {
		return write2(split_year_lower(_date.get_year()));
	}

	void on_abbr_weekday() {
		_out = detail::write<Char>(_out, _date.get_weekday_name().substr(0, 3));
	}

	void on_full_weekday() {
		_out = detail::write<Char>(_out, _date.get_weekday_name());
	}

	void on_dec0_weekday(numeric_system ns) {
		return write1(_date.get_day_of_week());
	}

	void on_dec1_weekday(numeric_system ns) {
		auto wday = _date.get_day_of_week();
		write1(wday == 0 ? DAYS_PER_WEEK : wday);
	}

	void on_abbr_month() {
		_out = detail::write<Char>(_out, _date.get_month_name().substr(0, 3));
	}
	void on_full_month() {
		_out = detail::write<Char>(_out, _date.get_month_name());
	}

	void on_dec_month(numeric_system ns, pad_type pad) {
		return write2(_date.get_month(), pad);
	}

	void on_dec0_week_of_year(numeric_system ns, pad_type pad) {
		return write2(_date.get_week_of_year(), pad);
	}

	void on_dec1_week_of_year(numeric_system ns, pad_type pad) {
		auto wday = _date.get_day_of_week();
		write2((_date.get_day_of_year() + DAYS_PER_WEEK - (wday == 0 ? (DAYS_PER_WEEK - 1) : (wday - 1))) / DAYS_PER_WEEK, pad);
	}

	void on_iso_week_of_year(numeric_system ns, pad_type pad) {
		return write2(date_iso_week_of_year(), pad);
	}

	void on_day_of_year(pad_type pad) {
		auto yday = _date.get_day_of_year() + 1;
		auto digit1 = yday / 100;
		if (digit1 != 0) {
			write1(digit1);
		} else {
			_out = write_padding(_out, pad);
		}
		write2(yday % 100, pad);
	}

	void on_day_of_month(numeric_system ns, pad_type pad) {
		return write2(_date.get_day(), pad);
	}

	void on_loc_date(numeric_system ns) {
		char buf[8];
		write_digit2_separated(
			buf, //
			detail::to_unsigned(_date.get_day()), //
			detail::to_unsigned(_date.get_month()), //
			detail::to_unsigned(split_year_lower(_date.get_year())), //
			'/'
		);
		_out = detail::copy<Char>(std::begin(buf), std::end(buf), _out);
	}

	void on_us_date() {
		char buf[8];
		write_digit2_separated(
			buf, //
			detail::to_unsigned(_date.get_month()), //
			detail::to_unsigned(_date.get_day()), //
			detail::to_unsigned(split_year_lower(_date.get_year())), //
			'/'
		);
		_out = detail::copy<Char>(std::begin(buf), std::end(buf), _out);
	}

	void on_iso_date() {
		auto year = _date.get_year();
		char buf[10];
		size_t offset = 0;
		if (year >= 0 && year < 10000) {
			detail::write2digits(buf, static_cast<size_t>(year / 100));
		} else {
			offset = 4;
			write_year_extended(year, pad_type::zero);
			year = 0;
		}
		write_digit2_separated(
			buf + 2, //
			static_cast<unsigned>(year % 100), //
			detail::to_unsigned(_date.get_month()), //
			detail::to_unsigned(_date.get_day()), //
			'-'
		);
		_out = detail::copy<Char>(std::begin(buf) + offset, std::end(buf), _out);
	}

private:
	static OutputIt write_padding(OutputIt out, pad_type pad, int width) {
		if (pad == pad_type::none) {
			return out;
		}
		return detail::fill_n(out, width, pad == pad_type::space ? ' ' : '0');
	}

	static OutputIt write_padding(OutputIt out, pad_type pad) {
		if (pad != pad_type::none) {
			*out++ = pad == pad_type::space ? ' ' : '0';
		}
		return out;
	}

	void write1(int value) {
		*_out++ = static_cast<char>('0' + detail::to_unsigned(value) % 10);
	}
	void write2(int value) {
		const char* d = detail::digits2(detail::to_unsigned(value) % 100);
		*_out++ = *d++;
		*_out++ = *d;
	}
	void write2(int value, pad_type pad) {
		unsigned int v = detail::to_unsigned(value) % 100;
		if (v >= 10) {
			const char* d = detail::digits2(v);
			*_out++ = *d++;
			*_out++ = *d;
		} else {
			_out = write_padding(_out, pad);
			*_out++ = static_cast<char>('0' + v);
		}
	}

	void write_year_extended(long long year, pad_type pad) {
		// At least 4 characters.
		int width = 4;
		bool negative = year < 0;
		if (negative) {
			year = 0 - year;
			--width;
		}
		detail::uint32_or_64_or_128_t<long long> n = detail::to_unsigned(year);
		const int num_digits = detail::count_digits(n);
		if (negative && pad == pad_type::zero) {
			*_out++ = '-';
		}
		if (width > num_digits) {
			_out = write_padding(_out, pad, width - num_digits);
		}
		if (negative && pad != pad_type::zero) {
			*_out++ = '-';
		}
		_out = detail::format_decimal<Char>(_out, n, num_digits);
	}
	void write_year(long long year, pad_type pad) {
		write_year_extended(year, pad);
	}

	int split_year_lower(long long year) const {
		auto l = year % 100;
		if (l < 0) {
			l = -l; // l in [0, 99]
		}
		return static_cast<int>(l);
	}

	// Writes two-digit numbers a, b and c separated by sep to buf.
	// The method by Pavel Novikov based on
	// https://johnnylee-sde.github.io/Fast-unsigned-integer-to-time-string/.
	static inline void write_digit2_separated(char* buf, unsigned a, unsigned b, unsigned c, char sep) {
		unsigned long long digits = a | (b << 24) | (static_cast<unsigned long long>(c) << 48);
		// Convert each value to BCD.
		// We have x = a * 10 + b and we want to convert it to BCD y = a * 16 + b.
		// The difference is
		//   y - x = a * 6
		// a can be found from x:
		//   a = floor(x / 10)
		// then
		//   y = x + a * 6 = x + floor(x / 10) * 6
		// floor(x / 10) is (x * 205) >> 11 (needs 16 bits).
		digits += (((digits * 205) >> 11) & 0x000f00000f00000f) * 6;
		// Put low nibbles to high bytes and high nibbles to low bytes.
		digits = ((digits & 0x00f00000f00000f0) >> 4) | ((digits & 0x000f00000f00000f) << 8);
		auto usep = static_cast<unsigned long long>(sep);
		// Add ASCII '0' to each digit byte and insert separators.
		digits |= 0x3030003030003030 | (usep << 16) | (usep << 40);

		constexpr const size_t len = 8;
		if (detail::const_check(detail::is_big_endian())) {
			char tmp[len];
			std::memcpy(tmp, &digits, len);
			std::reverse_copy(tmp, tmp + len, buf);
		} else {
			std::memcpy(buf, &digits, len);
		}
	}

	// Algorithm: https://en.wikipedia.org/wiki/ISO_week_date.
	int iso_year_weeks(long long curr_year) const {
		auto prev_year = curr_year - 1;
		auto curr_p = (curr_year + curr_year / 4 - curr_year / 100 + curr_year / 400) % DAYS_PER_WEEK;
		auto prev_p = (prev_year + prev_year / 4 - prev_year / 100 + prev_year / 400) % DAYS_PER_WEEK;
		return 52 + ((curr_p == 4 || prev_p == 3) ? 1 : 0);
	}
	int iso_week_num(int tm_yday, int tm_wday) const {
		return (tm_yday + 11 - (tm_wday == 0 ? DAYS_PER_WEEK : tm_wday)) / DAYS_PER_WEEK;
	}
	long long date_iso_week_year() const {
		auto year = _date.get_year();
		auto w = iso_week_num(_date.get_day_of_year(), _date.get_day_of_week());
		if (w < 1) {
			return year - 1;
		}
		if (w > iso_year_weeks(year)) {
			return year + 1;
		}
		return year;
	}
	int date_iso_week_of_year() const {
		auto year = _date.get_year();
		auto w = iso_week_num(_date.get_day_of_year(), _date.get_day_of_week());
		if (w < 1) {
			return iso_year_weeks(year - 1);
		}
		if (w > iso_year_weeks(year)) {
			return 1;
		}
		return w;
	}

	OutputIt _out;
	OpenVic::Date const& _date;
};

fmt::format_context::iterator fmt::formatter<Date>::format(Date d, format_context& ctx) const {
	format_specs specs { _specs };
	if (_specs.dynamic()) {
		detail::handle_dynamic_spec(_specs.dynamic_width(), specs.width, _specs.width_ref, ctx);
	}

	basic_memory_buffer buf = basic_memory_buffer<char>();
	basic_appender out = basic_appender<char>(buf);

	parse_date_format(_fmt.begin(), _fmt.end(), date_writer { out, d });
	return detail::write(ctx.out(), string_view { buf.data(), buf.size() }, specs);
}
