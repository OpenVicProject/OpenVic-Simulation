#pragma once

#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <limits>
#include <cstring>

#include "FPLUT.hpp"
#include "openvic/utility/FloatUtils.hpp"
#include "openvic/utility/StringUtils.hpp"
#include "openvic/utility/Logger.hpp"

namespace OpenVic {
	struct FP {
		static constexpr size_t SIZE = 8;

		constexpr FP() : value { 0 } {}
		constexpr FP(int64_t new_value) : value { new_value } {}
		constexpr FP(int32_t new_value) : value { static_cast<int64_t>(new_value) << FPLUT::PRECISION } {}

		// Trivial destructor
		~FP() = default;

		static constexpr FP max() {
			return std::numeric_limits<int64_t>::max();
		}

		static constexpr FP min() {
			return std::numeric_limits<int64_t>::min();
		}

		static constexpr FP usable_max() {
			return static_cast<int64_t>(2147483648LL);
		}

		static constexpr FP usable_min() {
			return -usable_max();
		}

		static constexpr FP _0() {
			return 0;
		}	

		static constexpr FP _1() {
			return 1;
		}

		static constexpr FP _2() {
			return 2;
		}

		static constexpr FP _3() {
			return 3;
		}

		static constexpr FP _4() {
			return 4;
		}

		static constexpr FP _5() {
			return 5;
		}

		static constexpr FP _6() {
			return 6;
		}

		static constexpr FP _7() {
			return 7;
		}

		static constexpr FP _8() {
			return 8;
		}

		static constexpr FP _9() {
			return 9;
		}

		static constexpr FP _10() {
			return 10;
		}

		static constexpr FP _50() {
			return 50;
		}

		static constexpr FP _100() {
			return 100;
		}

		static constexpr FP _200() {
			return 200;
		}

		static constexpr FP _0_01() {
			return _1() / _100();
		}

		static constexpr FP _0_02() {
			return _0_01() * 2;
		}

		static constexpr FP _0_03() {
			return _0_01() * 3;
		}

		static constexpr FP _0_04() {
			return _0_01() * 4;
		}

		static constexpr FP _0_05() {
			return _0_01() * 5;
		}

		static constexpr FP _0_10() {
			return _1() / 10;
		}

		static constexpr FP _0_20() {
			return _0_10() * 2;
		}

		static constexpr FP _0_25() {
			return _1() / 4;
		}

		static constexpr FP _0_33() {
			return _1() / 3;
		}

		static constexpr FP _0_50() {
			return _1() / 2;
		}

		static constexpr FP _0_75() {
			return _1() - _0_25();
		}

		static constexpr FP _0_95() {
			return _1() - _0_05();
		}

		static constexpr FP _0_99() {
			return _1() - _0_01();
		}

		static constexpr FP _1_01() {
			return _1() + _0_01();
		}

		static constexpr FP _1_10() {
			return _1() + _0_10();
		}

		static constexpr FP _1_50() {
			return _1() + _0_50();
		}

		static constexpr FP minus_one() {
			return -1;
		}

		static constexpr FP pi() {
			return static_cast<int64_t>(205887LL);
		}

		static constexpr FP pi2() {
			return pi() * 2;
		}

		static constexpr FP pi_quarter() {
			return pi() / 4;
		}

		static constexpr FP pi_half() {
			return pi() / 2;
		}

		static constexpr FP one_div_pi2() {
			return 1 / pi2();
		}

		static constexpr FP deg2rad() {
			return static_cast<int64_t>(1143LL);
		}

		static constexpr FP rad2deg() {
			return static_cast<int64_t>(3754936LL);
		}

		static constexpr FP e() {
			return static_cast<int64_t>(178145LL);
		}

		constexpr FP abs() const {
			return value >= 0 ? value : -value;
		}

		// Not including sign, so -1.1 -> 0.1
		constexpr FP get_frac() const {
			return abs().value & ((1 << FPLUT::PRECISION) - 1);
		}

		constexpr int64_t to_int64_t() const {
			return value >> FPLUT::PRECISION;
		}

		constexpr void set_raw_value(int64_t value) {
			this->value = value;
		}

		constexpr int64_t get_raw_value() const {
			return value;
		}

		constexpr int32_t to_int32_t() const {
			return static_cast<int32_t>(value >> FPLUT::PRECISION);
		}

		constexpr float to_float() const {
			return value / 65536.0F;
		}

		constexpr float to_float_rounded() const {
			return static_cast<float>(FloatUtils::round_to_int64((value / 65536.0F) * 100000.0F)) / 100000.0F;
		}

		constexpr float to_double() const {
			return value / 65536.0;
		}

		constexpr float to_double_rounded() const {
			return FloatUtils::round_to_int64((value / 65536.0) * 100000.0) / 100000.0;
		}

		std::string to_string() const {
			std::string str = std::to_string(to_int64_t()) + ".";
			FP frac_part = get_frac();
			do {
				frac_part.value *= 10;
				str += std::to_string(frac_part.to_int64_t());
				frac_part = frac_part.get_frac();
			} while (frac_part > 0);
			return str;
		}

		// Deterministic
		static constexpr FP parse_raw(int64_t value) {
			return value;
		}

		// Deterministic
		static constexpr FP parse(int64_t value) {
			return value << FPLUT::PRECISION;
		}

		// Deterministic
		static constexpr FP parse(const char* str, const char* end, bool *successful = nullptr) {
			if (successful != nullptr) *successful = false;

			if (str == nullptr || str >= end) {
				return _0();
			}

			bool negative = false;

			if (*str == '-') {
				negative = true;
				++str;
				if (str == end) return _0();
			}

			const char* dot_pointer = str;
			while (*dot_pointer != '.' && ++dot_pointer != end);

			if (dot_pointer == str && dot_pointer + 1 == end) {
				// Invalid: ".", "+." or "-."
				return _0();
			}

			FP result = _0();
			if (successful != nullptr) *successful = true;

			if (dot_pointer != str) {
				// Non-empty integer part
				bool int_successful = false;
				result += parse_integer(str, dot_pointer,  &int_successful);
				if (!int_successful && successful != nullptr) *successful = false;
			}

			if (dot_pointer + 1 < end) {
				// Non-empty fractional part
				bool frac_successful = false;
				result += parse_fraction(dot_pointer + 1, end,  &frac_successful);
				if (!frac_successful && successful != nullptr) *successful = false;
			}

			return negative ? -result : result;
		}

		static constexpr FP parse(const char* str, size_t length, bool *successful = nullptr) {
			return parse(str, str + length, successful);
		}

		// Not Deterministic
		static constexpr FP parse_unsafe(float value) {
			return static_cast<int64_t>(value * FPLUT::ONE + 0.5f * (value < 0 ? -1 : 1));
		}

		// Not Deterministic
		static FP parse_unsafe(const char* value) {
			char* endpointer;
			double double_value = std::strtod(value, &endpointer);

			if (*endpointer != '\0') {
				Logger::error("Unsafe fixed point parse failed to parse the end of a string: \"", endpointer, "\"");
			}

			int64_t integer_value = static_cast<long>(double_value * FPLUT::ONE + 0.5 * (double_value < 0 ? -1 : 1));

			return integer_value;
		}

		constexpr operator int32_t() const {
			return to_int32_t(); 
		}

		constexpr operator int64_t() const {
			return to_int64_t();
		}

		constexpr operator float() const {
			return to_float();
		}

		constexpr operator double() const {
			return to_double();
		}

		operator std::string() const {
			return to_string();
		}

		friend std::ostream& operator<<(std::ostream& stream, const FP& obj) {
			return stream << obj.to_string();
		}

		constexpr friend FP operator-(const FP& obj) {
			return -obj.value;
		}

		constexpr friend FP operator+(const FP& obj) {
			return +obj.value;
		}

		constexpr friend FP operator+(const FP& lhs, const FP& rhs) {
			return lhs.value + rhs.value;
		}

		constexpr friend FP operator+(const FP& lhs, const int32_t& rhs) {
			return lhs.value + (static_cast<int64_t>(rhs) << FPLUT::PRECISION);
		}

		constexpr friend FP operator+(const int32_t& lhs, const FP& rhs) {
			return (static_cast<int64_t>(lhs) << FPLUT::PRECISION) + rhs.value;
		}

		constexpr FP operator+=(const FP& obj) {
			value += obj.value;
			return *this;
		}

		constexpr FP operator+=(const int32_t& obj) {
			value += (static_cast<int64_t>(obj) << FPLUT::PRECISION);
			return *this;
		}

		constexpr friend FP operator-(const FP& lhs, const FP& rhs) {
			return lhs.value - rhs.value;
		}

		constexpr friend FP operator-(const FP& lhs, const int32_t& rhs) {
			return lhs.value - (static_cast<int64_t>(rhs) << FPLUT::PRECISION);
		}

		constexpr friend FP operator-(const int32_t& lhs, const FP& rhs) {
			return (static_cast<int64_t>(lhs) << FPLUT::PRECISION) - rhs.value;
		}

		constexpr FP operator-=(const FP& obj) {
			value -= obj.value;
			return *this;
		}

		constexpr FP operator-=(const int32_t& obj) {
			value -= (static_cast<int64_t>(obj) << FPLUT::PRECISION);
			return *this;
		}

		constexpr friend FP operator*(const FP& lhs, const FP& rhs) {
			return lhs.value * rhs.value >> FPLUT::PRECISION;
		}

		constexpr friend FP operator*(const FP& lhs, const int32_t& rhs) {
			return lhs.value * rhs;
		}

		constexpr friend FP operator*(const int32_t& lhs, const FP& rhs) {
			return lhs * rhs.value;
		}

		constexpr FP operator*=(const FP& obj) {
			value *= obj.value >> FPLUT::PRECISION;
			return *this;
		}

		constexpr FP operator*=(const int32_t& obj) {
			value *= obj;
			return *this;
		}

		constexpr friend FP operator/(const FP& lhs, const FP& rhs) {
			return (lhs.value  << FPLUT::PRECISION) / rhs.value;
		}

		constexpr friend FP operator/(const FP& lhs, const int32_t& rhs) {
			return lhs.value / rhs;
		}

		constexpr friend FP operator/(const int32_t& lhs, const FP& rhs) {
			return (static_cast<int64_t>(lhs) << (2 / FPLUT::PRECISION)) / rhs.value;
		}

		constexpr FP operator/=(const FP& obj) {
			value = (value << FPLUT::PRECISION) / obj.value;
			return *this;
		}

		constexpr FP operator/=(const int32_t& obj) {
			value /= obj;
			return *this;
		}

		constexpr friend FP operator%(const FP& lhs, const FP& rhs) {
			return lhs.value % rhs.value;
		}

		constexpr friend FP operator%(const FP& lhs, const int32_t& rhs) {
			return lhs.value % (static_cast<int64_t>(rhs) << FPLUT::PRECISION);
		}

		constexpr friend FP operator%(const int32_t& lhs, const FP& rhs) {
			return (static_cast<int64_t>(lhs) << FPLUT::PRECISION) % rhs.value;
		}

		constexpr FP operator%=(const FP& obj) {
			value %= obj.value;
			return *this;
		}

		constexpr FP operator%=(const int32_t& obj) {
			value %= (static_cast<int64_t>(obj) << FPLUT::PRECISION);
			return *this;
		}

		constexpr friend bool operator<(const FP& lhs, const FP& rhs) {
			return lhs.value < rhs.value;
		}

		constexpr friend bool operator<(const FP& lhs, const int32_t& rhs) {
			return lhs.value < static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator<(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION < rhs.value;
		}

		constexpr friend bool operator<=(const FP& lhs, const FP& rhs) {
			return lhs.value <= rhs.value;
		}

		constexpr friend bool operator<=(const FP& lhs, const int32_t& rhs) {
			return lhs.value <= static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator<=(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION <= rhs.value;
		}

		constexpr friend bool operator>(const FP& lhs, const FP& rhs) {
			return lhs.value > rhs.value;
		}

		constexpr friend bool operator>(const FP& lhs, const int32_t& rhs) {
			return lhs.value > static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator>(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION > rhs.value;
		}

		constexpr friend bool operator>=(const FP& lhs, const FP& rhs) {
			return lhs.value >= rhs.value;
		}

		constexpr friend bool operator>=(const FP& lhs, const int32_t& rhs) {
			return lhs.value >= static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator>=(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION >= rhs.value;
		}

		constexpr friend bool operator==(const FP& lhs, const FP& rhs) {
			return lhs.value == rhs.value;
		}

		constexpr friend bool operator==(const FP& lhs, const int32_t& rhs) {
			return lhs.value == static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator==(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION == rhs.value;
		}

		constexpr friend bool operator!=(const FP& lhs, const FP& rhs) {
			return lhs.value != rhs.value;
		}

		constexpr friend bool operator!=(const FP& lhs, const int32_t& rhs) {
			return lhs.value != static_cast<int64_t>(rhs) << FPLUT::PRECISION;
		}

		constexpr friend bool operator!=(const int32_t& lhs, const FP& rhs) {
			return static_cast<int64_t>(lhs) << FPLUT::PRECISION != rhs.value;
		}


	private:
		int64_t value;

		static constexpr FP parse_integer(const char* str, const char* end, bool* successful) {
			int64_t parsed_value = StringUtils::string_to_int64(str, end, successful, 10);
			return parse(parsed_value);
		}

		static constexpr FP parse_fraction(const char* str, const char* end, bool* successful) {
			constexpr size_t READ_SIZE = 5;
			if (str + READ_SIZE < end) {
				Logger::error("Fixed point fraction parse will only use the first ", READ_SIZE,
					" characters of ", std::string_view { str, static_cast<size_t>(end - str) });
				end = str + READ_SIZE;
			}
			int64_t parsed_value = StringUtils::string_to_int64(str, end, successful, 10);
			return parse_raw(parsed_value * 65536 / 100000);
		}
	};

	static_assert(sizeof(FP) == FP::SIZE, "FP is not 8 bytes");
}
