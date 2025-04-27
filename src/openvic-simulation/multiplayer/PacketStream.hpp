#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "openvic-simulation/multiplayer/NetworkError.hpp"

namespace OpenVic {
	struct PacketStream {
		virtual NetworkError set_data(std::span<uint8_t> buffer) = 0;
		virtual NetworkError get_data(std::span<uint8_t> r_buffer) = 0;
		virtual NetworkError set_data_no_blocking(std::span<uint8_t> buffer, size_t& r_sent) = 0;
		virtual NetworkError get_data_no_blocking(std::span<uint8_t> r_buffer, size_t& r_received) = 0;
		virtual int64_t available_bytes() const = 0;

		void set_big_endian(bool endian);
		bool is_big_endian() const;

		void put_8(int8_t p_val);
		void put_u8(uint8_t p_val);
		void put_16(int16_t p_val);
		void put_u16(uint16_t p_val);
		void put_32(int32_t p_val);
		void put_u32(uint32_t p_val);
		void put_64(int64_t p_val);
		void put_u64(uint64_t p_val);
		void put_half(float p_val);
		void put_float(float p_val);
		void put_double(double p_val);
		void put_string(std::string_view p_string);

		uint8_t get_u8();
		int8_t get_8();
		uint16_t get_u16();
		int16_t get_16();
		uint32_t get_u32();
		int32_t get_32();
		uint64_t get_u64();
		int64_t get_64();
		float get_half();
		float get_float();
		double get_double();
		std::string get_string(int64_t bytes = -1);

	protected:
		bool _is_big_endian = std::endian::native == std::endian::big;
	};

	struct BufferPacketStream : PacketStream {
		NetworkError set_data(std::span<uint8_t> buffer) override;
		NetworkError get_data(std::span<uint8_t> r_buffer) override;

		NetworkError set_data_no_blocking(std::span<uint8_t> buffer, size_t& r_sent) override;
		NetworkError get_data_no_blocking(std::span<uint8_t> r_buffer, size_t& r_received) override;

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
		std::vector<uint8_t> _data;
		size_t _position;
	};
}
