#pragma once

#include <cstdint>
#include <span>

#include <zpp_bits.h>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"

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

		zpp::bits::in<std::span<uint8_t>> in() {
			return zpp::bits::in(get_packet());
		}

	protected:
		virtual NetworkError _set_packet(std::span<uint8_t> buffer) = 0;
		virtual NetworkError _get_packet(std::span<uint8_t>& r_set) = 0;

		mutable NetworkError _last_error = NetworkError::OK;
	};
}
