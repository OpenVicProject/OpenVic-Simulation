#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <vector>

#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	template<std::endian DefaultEndian = std::endian::native>
	struct PacketBuilder : memory::vector<uint8_t> {
		using base_type = memory::vector<uint8_t>;
		using base_type::base_type;

		template<typename T, std::endian Endian = DefaultEndian>
		requires requires(T const& value) { utility::encode<T, Endian>(value, std::span<uint8_t> {}); }
		void put_back(T const& value) {
			size_t value_size = utility::encode<T, Endian>(value);
			size_t last = size();
			resize(size() + value_size);
			utility::encode<T, Endian>(value, std::span<uint8_t> { data() + last, value_size });
		}
	};

	extern template struct PacketBuilder<>;
}
