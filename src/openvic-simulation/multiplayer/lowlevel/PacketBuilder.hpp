#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <vector>

#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	template<std::endian DefaultEndian = std::endian::native>
	struct PacketBuilder : std::vector<uint8_t> {
		using base_type = std::vector<uint8_t>;
		using base_type::base_type;

		template<typename T, std::endian Endian = DefaultEndian>
		requires requires(T value) { utility::encode<T, Endian>(value, std::span<uint8_t> {}); }
		void put_back(T value) {
			size_t value_size = utility::encode<T, Endian>(value);
			size_t last = size();
			resize(size() + value_size);
			utility::encode<T, Endian>(value, std::span<uint8_t> { data() + last, value_size });
		}
	};

	extern template struct PacketBuilder<>;
}
