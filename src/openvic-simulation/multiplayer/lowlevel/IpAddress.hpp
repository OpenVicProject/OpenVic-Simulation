#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

namespace OpenVic {
	struct IpAddress {
		constexpr IpAddress() : _field {} {
			clear();
		}

		constexpr IpAddress(char const* cstring) : IpAddress(std::string_view { cstring }) {}

		constexpr IpAddress(std::string_view string) : _field {} {
			clear();
			from_chars(string.data(), string.data() + string.size());
		}

		constexpr IpAddress(uint32_t a, uint32_t b, uint32_t c, uint32_t d, bool is_ipv6 = false) : _field {} {
			clear();
			_valid = true;
			if (is_ipv6) {
				auto _set_buffer = [](uint8_t* buffer, uint32_t value) constexpr {
					buffer[0] = (value >> 24) & 0xff;
					buffer[1] = (value >> 16) & 0xff;
					buffer[2] = (value >> 8) & 0xff;
					buffer[3] = (value >> 0) & 0xff;
				};
				_set_buffer(&_field._8bit[0], a);
				_set_buffer(&_field._8bit[4], b);
				_set_buffer(&_field._8bit[8], c);
				_set_buffer(&_field._8bit[12], d);
				return;
			}
			if (std::is_constant_evaluated()) {
				_field._8bit[10] = 0xff;
				_field._8bit[11] = 0xff;
			} else {
				_field._16bit[5] = 0xffff;
			}
			_field._8bit[12] = a;
			_field._8bit[13] = b;
			_field._8bit[14] = c;
			_field._8bit[15] = d;
		}

		constexpr void clear() {
			if (std::is_constant_evaluated()) {
				std::fill_n(_field._8bit.data(), sizeof(_field._8bit), 0);
			} else {
				std::memset(_field._8bit.data(), 0, sizeof(_field._8bit));
			}
			_wildcard = false;
			_valid = false;
		}

		constexpr bool is_wildcard() const {
			return _wildcard;
		}

		constexpr bool is_valid() const {
			return _valid;
		}

		constexpr bool is_ipv4() const {
			if (std::is_constant_evaluated()) {
				std::array<uint16_t, 8> casted_fields = std::bit_cast<std::array<uint16_t, 8>>(_field._8bit);
				return casted_fields[0] == 0 && casted_fields[1] == 0 && casted_fields[2] == 0 && casted_fields[3] == 0 &&
					casted_fields[4] == 0 && casted_fields[5] == 0xffff;
			} else {
				return (_field._32bit[0] == 0 && _field._32bit[1] == 0 && _field._16bit[4] == 0 && _field._16bit[5] == 0xffff);
			}
		}

		constexpr std::span<const uint8_t, 4> get_ipv4() const {
			if (std::is_constant_evaluated()) {
				return std::span<const uint8_t> { _field._8bit }.subspan<12, 4>();
			} else {
				OV_ERR_FAIL_COND_V_MSG(
					!is_ipv4(), (std::span<const uint8_t> { _field._8bit }.subspan<12, 4>()),
					"IPv4 requested, but current IP is IPv6."
				);
			}
			return std::span<const uint8_t> { _field._8bit }.subspan<12, 4>();
		}

		constexpr void set_ipv4(std::span<const uint8_t, 4> ip) {
			clear();
			_valid = true;
			if (std::is_constant_evaluated()) {
				_field._8bit[10] = 0xff;
				_field._8bit[11] = 0xff;
				std::copy_n(ip.data(), 4, &_field._8bit[12]);
			} else {
				_field._16bit[5] = 0xffff;
				_field._32bit[3] = *std::bit_cast<uint32_t const*>(ip.data());
			}
		}

		constexpr void set_ipv4(std::initializer_list<const uint8_t> list) {
			set_ipv4(std::span<const uint8_t, 4> { list });
		}

		constexpr std::span<const uint8_t, 16> get_ipv6() const {
			return _field._8bit;
		}

		constexpr void set_ipv6(std::span<const uint8_t, 16> ip) {
			clear();
			_valid = true;
			for (size_t i = 0; i < _field._8bit.size(); i++) {
				_field._8bit[i] = ip[i];
			}
		}

		constexpr void set_ipv6(std::initializer_list<const uint8_t> list) {
			set_ipv6(std::span<const uint8_t, 16> { list });
		}

		constexpr bool operator==(IpAddress const& ip) const {
			if (ip._valid != _valid) {
				return false;
			}
			if (!_valid) {
				return false;
			}
			if (std::is_constant_evaluated()) {
				for (int i = 0; i < _field._8bit.size(); i++) {
					if (_field._8bit[i] != ip._field._8bit[i]) {
						return false;
					}
				}
			} else {
				for (int i = 0; i < _field._32bit.size(); i++) {
					if (_field._32bit[i] != ip._field._32bit[i]) {
						return false;
					}
				}
			}
			return true;
		}

		enum class to_chars_option : uint8_t { //
			SHORTEN_IPV6,
			COMPRESS_IPV6,
			EXPAND_IPV6,
		};

		inline constexpr std::to_chars_result to_chars( //
			char* first, char* last, bool prefer_ipv4 = true, to_chars_option option = to_chars_option::SHORTEN_IPV6
		) const {
			if (first == nullptr || first >= last) {
				return { last, std::errc::value_too_large };
			}

			if (_wildcard) {
				*first = '*';
				++first;
				return { last, std::errc {} };
			}

			if (!_valid) {
				return { last, std::errc {} };
			}

			std::to_chars_result result = { first, std::errc {} };
			if (is_ipv4() && prefer_ipv4) {
				for (size_t i = 12; i < 16; i++) {
					if (i > 12) {
						if (last - result.ptr <= 1) {
							return { last, std::errc::value_too_large };
						}

						*result.ptr = '.';
						++result.ptr;
					}

					result = StringUtils::to_chars(result.ptr, last, _field._8bit[i]);
					if (result.ec != std::errc {}) {
						return result;
					}
				}
				return result;
			}

			auto section_func = [this](size_t index) constexpr -> uint16_t {
				return (_field._8bit[index * 2] << 8) + _field._8bit[index * 2 + 1];
			};

			int32_t compress_pos = -1;
			if (option == to_chars_option::COMPRESS_IPV6) {
				int32_t last_compress_count = 0;
				for (size_t i = 0; i < 8; i++) {
					if (_field._8bit[i * 2] == 0 && _field._8bit[i * 2 + 1] == 0 && section_func(i + 1) == 0) {
						int32_t compress_check = i;
						do {
							++i;
						} while (i < 8 && section_func(i) == 0);

						if (int32_t compress_count = i - compress_check;
							compress_count >= 2 && last_compress_count < compress_count) {
							compress_pos = compress_check;
							last_compress_count = compress_count;
						}
					}
				}
			}

			for (size_t i = 0; i < 8; i++) {
				if (compress_pos > -1 && compress_pos == i) {
					if (last - result.ptr <= 2) {
						return { last, std::errc::value_too_large };
					}

					result.ptr[0] = ':';
					result.ptr[1] = ':';
					result.ptr += 2;
					do {
						++i;
					} while (i < 8 && section_func(i) == 0);
				} else if (i > 0) {
					*result.ptr = ':';
					++result.ptr;
				}

				uint16_t section = section_func(i);
				if (option == to_chars_option::EXPAND_IPV6) {
					if (last - result.ptr < 4) {
						return { last, std::errc::value_too_large };
					}

					if (section < 0xFFF) {
						*result.ptr = '0';
						++result.ptr;
					}
					if (section < 0xFF) {
						*result.ptr = '0';
						++result.ptr;
					}
					if (section < 0xF) {
						*result.ptr = '0';
						++result.ptr;
					}
					if (section == 0) {
						*result.ptr = '0';
						++result.ptr;
						continue;
					}
				}
				result = StringUtils::to_chars(result.ptr, last, section, 16);
				if (result.ec != std::errc {}) {
					return result;
				}
			}
			return result;
		}

		struct stack_string;
		inline constexpr stack_string to_array( //
			bool prefer_ipv4 = true, to_chars_option option = to_chars_option::SHORTEN_IPV6
		) const;

		struct stack_string final : StackString<39> {
		protected:
			using StackString::StackString;
			friend inline constexpr stack_string IpAddress::to_array(bool prefer_ipv4, to_chars_option option) const;
		};

		memory::string to_string(bool prefer_ipv4 = true, to_chars_option option = to_chars_option::SHORTEN_IPV6) const;
		explicit operator memory::string() const;

		constexpr std::from_chars_result from_chars(char const* begin, char const* end) {
			if (begin == nullptr || begin >= end) {
				return { begin, std::errc::invalid_argument };
			}

			if (*begin == '*') {
				_wildcard = true;
				_valid = false;
				return { begin + 1, std::errc {} };
			}

			size_t check_for_ipv4 = 0;
			for (char const* check = begin; check < end; check++) {
				if (*check == '.') {
					check_for_ipv4++;
				}
			}

			fields_type fields { ._8bit {} };

			std::from_chars_result result = { begin, std::errc {} };
			if (check_for_ipv4 == 3) {
				if (std::is_constant_evaluated()) {
					fields._8bit[10] = 0xff;
					fields._8bit[11] = 0xff;
				} else {
					fields._16bit[5] = 0xffff;
				}
				for (size_t i = 0; i < 4; i++) {
					if (*result.ptr == '.') {
						if (i > 0 && end - result.ptr >= 1) {
							++result.ptr;
						} else {
							return { begin, std::errc::invalid_argument };
						}
					}

					result = StringUtils::from_chars(result.ptr, end, fields._8bit[12 + i]);
					if (result.ec != std::errc {}) {
						return result;
					}
				}
				_field._8bit.swap(fields._8bit);
				_valid = true;
				return result;
			} else if (check_for_ipv4 > 0) {
				return { begin, std::errc::invalid_argument };
			}

			int32_t compress_start = -1;
			size_t colon_count = 0;
			if (std::is_constant_evaluated()) {
				fields._16bit = std::bit_cast<std::array<uint16_t, 8>>(fields._8bit);
			}
			for (size_t i = 0; i < fields._16bit.size(); i++) {
				if (*result.ptr == ':') {
					if (end - result.ptr <= 2) {
						return { result.ptr, std::errc::invalid_argument };
					}

					++result.ptr;
					if (*result.ptr == ':') {
						if (compress_start > -1 || i >= fields._16bit.size() - 2) {
							return { result.ptr, std::errc::invalid_argument };
						}

						++result.ptr;
						compress_start = i;
					} else {
						++colon_count;
					}
				}

				uint16_t big_endian_value;
				result = StringUtils::from_chars(result.ptr, end, big_endian_value, 16);
				fields._16bit[i] = (big_endian_value >> 8) | (big_endian_value << 8);
				if (result.ec != std::errc {}) {
					return result;
				}
			}

			if (compress_start > -1) {
				constexpr size_t expected_ipv6_colons = 7;
				size_t index = compress_start - 1;
				// Pre-compression
				std::swap_ranges(_field._16bit.data(), _field._16bit.data() + index, fields._16bit.data());
				size_t compress_end = expected_ipv6_colons - colon_count - 1;
				// Compression
				if (std::is_constant_evaluated()) {
					std::fill_n(_field._16bit.data() + compress_start, compress_end - 1, 0);
				} else {
					std::memset(_field._16bit.data() + compress_start, 0, compress_end - 1);
				}
				// Post-compression
				std::swap_ranges(
					_field._16bit.data() + compress_end, fields._16bit.data() + fields._16bit.size(),
					fields._16bit.data() + index
				);
			} else {
				if (std::is_constant_evaluated()) {
					_field._16bit = std::bit_cast<std::array<uint16_t, 8>>(_field._8bit);
				}
				_field._16bit.swap(fields._16bit);
			}

			if (std::is_constant_evaluated()) {
				_field._8bit = std::bit_cast<std::array<uint8_t, 16>>(_field._16bit);
			}

			_valid = true;
			return result;
		}

	private:
		union fields_type {
			std::array<uint8_t, 16> _8bit;
			std::array<uint16_t, 8> _16bit;
			std::array<uint32_t, 4> _32bit;
		} _field;

		bool _wildcard = false;
		bool _valid = false;
	};

	inline constexpr IpAddress::stack_string IpAddress::to_array(bool pref_ipv4, to_chars_option option) const {
		stack_string str {};
		std::to_chars_result result = to_chars(str._array.data(), str._array.data() + str._array.size(), pref_ipv4, option);
		str._string_size = result.ptr - str.data();
		return str;
	}
}
