#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	template<typename Container, std::endian DefaultEndian = std::endian::native>
	requires requires(Container& container) {
		{ container.data() } -> std::same_as<uint8_t*>;
		{ container.size() } -> std::same_as<size_t>;
	}
	struct PacketReaderAdapter : Container {
		using base_type = Container;
		using base_type::base_type;

		static constexpr std::integral_constant<size_t, std::numeric_limits<size_t>::max()> npos {};

		size_t index() const {
			return _read_index;
		}

		void seek(size_t index) {
			if (index == npos) {
				_read_index = npos;
			}
			OV_ERR_FAIL_INDEX(index, this->size());
			_read_index = index;
		}

		template<typename T, std::endian Endian = DefaultEndian>
		requires requires(PacketReaderAdapter* self, size_t& size) {
			{
				utility::decode<T, Endian>(
					std::span<uint8_t> { self->data() + self->_read_index, self->size() - self->_read_index }, size
				)
			} -> std::same_as<std::remove_cv_t<T>>;
		}
		T read() {
			size_t decode_size = 0;
			T result = utility::decode<T, Endian>(
				std::span<uint8_t> { this->data() + _read_index, this->size() - _read_index }, decode_size
			);
			OV_ERR_FAIL_COND_V(decode_size > this->size(), {});
			_read_index += decode_size;
			return result;
		}

	private:
		size_t _read_index = 0;
	};

	template<std::endian Endian = std::endian::native>
	using PacketBufferEndian = PacketReaderAdapter<memory::vector<uint8_t>, Endian>;
	template<std::endian Endian = std::endian::native>
	using PacketSpanEndian = PacketReaderAdapter<std::span<uint8_t>, Endian>;

	using PacketBuffer = PacketBufferEndian<>;
	using PacketSpan = PacketSpanEndian<>;

	extern template struct PacketReaderAdapter<memory::vector<uint8_t>>;
	extern template struct PacketReaderAdapter<std::span<uint8_t>>;
}
