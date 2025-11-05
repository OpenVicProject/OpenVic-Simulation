#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "openvic-simulation/core/memory/String.hpp"

namespace OpenVic {
	template<size_t Size>
	struct StackString {
		static constexpr size_t array_length = Size;

	protected:
		std::array<char, array_length> _array {};
		uint8_t _string_size = 0;

		constexpr StackString() {}

	public:
		constexpr const char* data() const {
			return _array.data();
		}

		constexpr size_t size() const {
			return _string_size;
		}

		constexpr size_t length() const {
			return _string_size;
		}

		constexpr decltype(_array)::const_iterator begin() const {
			return _array.begin();
		}

		constexpr decltype(_array)::const_iterator end() const {
			return begin() + size();
		}

		constexpr decltype(_array)::const_reference operator[](size_t index) const {
			return _array[index];
		}

		constexpr bool empty() const {
			return size() == 0;
		}

		constexpr operator std::string_view() const {
			return std::string_view { data(), data() + size() };
		}

		operator memory::string() const {
			return memory::string { data(), size() };
		}
	};
}
