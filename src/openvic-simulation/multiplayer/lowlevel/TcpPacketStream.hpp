#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketStream.hpp"

namespace OpenVic {
	struct TcpPacketStream : PacketStream {
		enum class Status : uint8_t {
			NONE,
			CONNECTING,
			CONNECTED,
			ERROR,
		};

		TcpPacketStream();
		~TcpPacketStream();

		void accept_socket(std::shared_ptr<NetworkSocket> p_sock, IpAddress p_host, NetworkSocket::port_type p_port);

		int64_t available_bytes() const override;
		NetworkError bind(NetworkSocket::port_type port, IpAddress const& address = "*");
		NetworkError connect_to(IpAddress const& host, NetworkSocket::port_type port);
		NetworkError poll();
		NetworkError wait(NetworkSocket::PollType p_type, int p_timeout = 0);
		void close();
		NetworkSocket::port_type get_bound_port() const;
		bool is_connected() const;

		IpAddress get_connected_address() const;
		NetworkSocket::port_type get_connected_port() const;
		Status get_status() const;

		void set_timeout_seconds(uint64_t timeout);
		uint64_t get_timeout_seconds() const;

		void set_no_delay(bool p_enabled);

		NetworkError set_data(std::span<const uint8_t> buffer) override;
		NetworkError set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) override;
		NetworkError get_data(std::span<uint8_t> buffer_to_set) override;
		NetworkError get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) override;

	private:
		uint64_t _timeout_seconds = 30;

		Status _status = Status::NONE;
		uint64_t _timeout = 0;
		NetworkSocket::port_type _client_port = 0;
		IpAddress _client_address;
		std::shared_ptr<NetworkSocket> _socket;

		template<bool IsBlocking>
		NetworkError _set_data(std::span<const uint8_t> buffer, size_t& r_sent);

		template<bool IsBlocking>
		NetworkError _get_data(std::span<uint8_t>& r_buffer);
	};

	extern template NetworkError TcpPacketStream::_set_data<true>(std::span<const uint8_t> buffer, size_t& r_sent);
	extern template NetworkError TcpPacketStream::_set_data<false>(std::span<const uint8_t> buffer, size_t& r_sent);
	extern template NetworkError TcpPacketStream::_get_data<true>(std::span<uint8_t>& r_buffer);
	extern template NetworkError TcpPacketStream::_get_data<false>(std::span<uint8_t>& r_buffer);
}
