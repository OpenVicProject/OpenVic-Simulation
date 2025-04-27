#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <vector>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct PacketStream {
		virtual NetworkError set_data(std::span<const uint8_t> buffer) = 0;
		virtual NetworkError get_data(std::span<uint8_t> buffer_to_set) = 0;
		virtual NetworkError set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) = 0;
		virtual NetworkError get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) = 0;
		virtual int64_t available_bytes() const = 0;

		template<std::endian Endian = std::endian::native>
		PacketBufferEndian<Endian> packet_buffer(size_t bytes) {
			PacketBufferEndian<Endian> result;
			result.resize(bytes);
			return packet_buffer(result);
		}

		template<std::endian Endian = std::endian::native>
		PacketBufferEndian<Endian> packet_buffer(PacketBufferEndian<Endian>& buffer_store) {
			get_data(buffer_store);
			return buffer_store;
		}
	};

	struct BufferPacketStream : PacketStream {
		NetworkError set_data(std::span<const uint8_t> buffer) override;
		NetworkError get_data(std::span<uint8_t> r_buffer) override;

		NetworkError set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) override;
		NetworkError get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) override;

		virtual int64_t available_bytes() const override;

		void seek(size_t p_pos);
		size_t size() const;
		size_t position() const;
		void resize(size_t p_size);

		void set_buffer_data(std::span<uint8_t> p_data);
		std::span<const uint8_t> get_buffer_data() const;

		void clear();

		BufferPacketStream duplicate() const;

	private:
		memory::vector<uint8_t> _data;
		size_t _position;
	};
}
