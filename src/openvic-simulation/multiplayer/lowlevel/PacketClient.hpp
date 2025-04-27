#pragma once

#include <cstdint>
#include <span>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"

namespace OpenVic {
	struct PacketClient {
		virtual int64_t available_packets() const = 0;
		virtual int64_t max_packet_size() const = 0;

		std::span<uint8_t> get_packet() {
			std::span<uint8_t> buffer;
			if (NetworkError error = _get_packet(buffer); error != NetworkError::OK) {
				_last_error = error;
				return {};
			}
			return buffer;
		}

		NetworkError set_packet(std::span<uint8_t> buffer) {
			NetworkError error = _set_packet(buffer);
			if (error != NetworkError::OK) {
				_last_error = error;
			}
			return error;
		}

		NetworkError get_last_error() const {
			return _last_error;
		}

		PacketSpan packet_span() {
			std::span<uint8_t> span = get_packet();
			return { span.data(), span.size() };
		}

	protected:
		virtual NetworkError _set_packet(std::span<uint8_t> buffer) = 0;
		virtual NetworkError _get_packet(std::span<uint8_t>& r_set) = 0;

		mutable NetworkError _last_error = NetworkError::OK;
	};
}
