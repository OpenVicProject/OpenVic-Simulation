#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <vector>

#include <range/v3/view/transform.hpp>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketStream.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"
#include "openvic-simulation/types/RingBuffer.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

namespace OpenVic {
	template<std::derived_from<PacketStream> T>
	struct BasicStreamPacketClient : PacketClient {
		BasicStreamPacketClient(uint8_t max_buffer_power = 16) {
			_ring_buffer.reserve_power(max_buffer_power);
			_input_buffer.resize(int64_t(1) << max_buffer_power);
			_output_buffer.resize(int64_t(1) << max_buffer_power);
		}

		int64_t available_packets() const override {
			_poll_buffer();

			uint32_t remaining = _ring_buffer.size() - _ring_buffer.space() - 1;

			int offset = 0;
			int count = 0;

			while (remaining >= 4) {
				uint8_t lbuf[4];
				std::copy_n(_ring_buffer.begin() + offset, 4, lbuf);
				size_t decode_size = 0;
				uint32_t length = utility::decode<uint32_t>(lbuf, decode_size);
				OV_ERR_BREAK(decode_size != 4);
				remaining -= 4;
				offset += 4;
				if (length > remaining) {
					break;
				}
				remaining -= length;
				offset += length;
				count++;
			}

			return count;
		}

		int64_t max_packet_size() const override {
			return _output_buffer.size();
		}

		void set_packet_stream(std::shared_ptr<T> stream) {
			if (stream != _stream) {
				_ring_buffer.clear(); // Reset the ring buffer.
			}

			_stream = stream;
		}

		std::shared_ptr<T> get_packet_stream() const {
			return _stream;
		}

		void set_input_buffer_max_size(int p_max_size) {
			OV_ERR_FAIL_COND_MSG(p_max_size < 0, "Max size of input buffer size cannot be smaller than 0.");
			// WARNING: May lose packets.
			OV_ERR_FAIL_COND_MSG(
				_ring_buffer.size() - _ring_buffer.space() - 1, "Buffer in use, resizing would cause loss of data."
			);
			_ring_buffer.reserve_power(std::bit_width<uint8_t>(std::bit_ceil<uint64_t>(p_max_size + 4)) - 1);
			_input_buffer.resize(std::bit_ceil<uint64_t>(p_max_size + 4));
		}

		int get_input_buffer_max_size() const {
			return _input_buffer.size() - 4;
		}

		void set_output_buffer_max_size(int p_max_size) {
			_output_buffer.resize(std::bit_ceil<uint64_t>(p_max_size + 4));
		}

		int get_output_buffer_max_size() const {
			return _output_buffer.size() - 4;
		}

	private:
		NetworkError _set_packet(std::span<uint8_t> buffer) override {
			OV_ERR_FAIL_COND_V(_stream == nullptr, NetworkError::UNCONFIGURED);
			NetworkError err = _poll_buffer(); // won't hurt to poll here too

			if (err != NetworkError::OK) {
				return err;
			}

			if (buffer.size() == 0) {
				return NetworkError::OK;
			}

			OV_ERR_FAIL_COND_V(buffer.size() + 4 > _output_buffer.size(), NetworkError::INVALID_PARAMETER);

			utility::encode<uint32_t>(buffer.size(), _output_buffer);
			uint8_t* dst = &_output_buffer[4];
			for (int i = 0; i < buffer.size(); i++) {
				dst[i] = buffer[i];
			}

			return _stream->set_data({ &_output_buffer[0], buffer.size() + 4 });
		}

		NetworkError _get_packet(std::span<uint8_t>& r_set) override {
			OV_ERR_FAIL_COND_V(_stream == nullptr, NetworkError::UNCONFIGURED);
			_poll_buffer();

			int remaining = _ring_buffer.size() - _ring_buffer.space() - 1;
			OV_ERR_FAIL_COND_V(remaining < 4, NetworkError::UNAVAILABLE);
			uint8_t lbuf[4];
			std::copy_n(_ring_buffer.begin(), 4, lbuf);
			remaining -= 4;
			size_t decode_size = 0;
			uint32_t length = utility::decode<uint32_t>(lbuf, decode_size);
			OV_ERR_FAIL_COND_V(remaining < (int)length, NetworkError::UNAVAILABLE);

			OV_ERR_FAIL_COND_V(_input_buffer.size() < length, NetworkError::UNAVAILABLE);
			_ring_buffer.erase(_ring_buffer.begin(), decode_size); // get rid of the decode_size
			_ring_buffer.read_buffer_to(_input_buffer.data(), length); // read packet

			r_set = std::span<uint8_t> { _input_buffer.data(), length };
			return NetworkError::OK;
		}

		NetworkError _poll_buffer() const {
			OV_ERR_FAIL_COND_V(_stream == nullptr, NetworkError::UNCONFIGURED);

			size_t read = 0;
			OV_ERR_FAIL_COND_V(_input_buffer.size() < _ring_buffer.space(), NetworkError::UNAVAILABLE);
			NetworkError err = _stream->get_data_no_blocking({ _input_buffer.data(), _ring_buffer.space() }, read);
			if (err != NetworkError::OK) {
				return err;
			}
			if (read == 0) {
				return NetworkError::OK;
			}

			typename decltype(_ring_buffer)::iterator last = _ring_buffer.end();
			ptrdiff_t w = std::distance(last, _ring_buffer.append(&_input_buffer[0], read));
			OV_ERR_FAIL_COND_V(w != read, NetworkError::BUG);

			return NetworkError::OK;
		}

		mutable std::shared_ptr<T> _stream;
		mutable RingBuffer<uint8_t> _ring_buffer;
		mutable memory::vector<uint8_t> _input_buffer;
		mutable memory::vector<uint8_t> _output_buffer;
	};

	using StreamPacketClient = BasicStreamPacketClient<PacketStream>;
	extern template struct BasicStreamPacketClient<PacketStream>;

	using TcpStreamPacketClient = BasicStreamPacketClient<TcpPacketStream>;
	extern template struct BasicStreamPacketClient<TcpPacketStream>;
}
