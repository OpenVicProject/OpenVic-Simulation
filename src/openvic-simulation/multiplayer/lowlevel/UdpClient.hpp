#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "openvic-simulation/multiplayer/lowlevel/HostnameAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketClient.hpp"
#include "openvic-simulation/types/RingBuffer.hpp"

namespace OpenVic {
	struct UdpServer;
	struct ReliableUdpServer;

	struct UdpClient : PacketClient {
		static constexpr size_t PACKET_BUFFER_SIZE = 65536;

		static constexpr size_t MAX_PACKET_SIZE = 512 * 3;

		UdpClient();
		~UdpClient();

		UdpClient(UdpClient&&) = default;
		UdpClient& operator=(UdpClient&&) = default;

		void close();
		NetworkSocket::port_type get_bound_port() const;
		bool is_connected() const;

		NetworkError connect_to(IpAddress const& host, NetworkSocket::port_type port);
		NetworkError bind( //
			NetworkSocket::port_type port, IpAddress const& address, size_t min_buffer_size = PACKET_BUFFER_SIZE
		);
		IpAddress get_last_packet_ip() const;
		NetworkSocket::port_type get_last_packet_port() const;
		bool is_bound() const;
		NetworkError wait();
		NetworkError set_destination(HostnameAddress const& addr, NetworkSocket::port_type port);
		virtual NetworkError store_packet(IpAddress p_ip, NetworkSocket::port_type p_port, std::span<uint8_t> p_buf);

		NetworkError join_multicast_group(IpAddress addr, std::string_view interface_name);
		NetworkError leave_multicast_group(IpAddress addr, std::string_view interface_name);

		bool is_broadcast_enabled() const;
		void set_broadcast_enabled(bool enabled);

		bool is_blocking() const;
		void set_blocking(bool blocking);

		int64_t available_packets() const override;
		int64_t max_packet_size() const override;

	protected:
		NetworkError _set_packet(std::span<uint8_t> buffer) override;
		NetworkError _get_packet(std::span<uint8_t>& r_set) override;

		NetworkError _poll();

		bool _is_connected = false;
		bool _is_blocking = true;
		bool _is_broadcasting = false;
		NetworkSocket::port_type _packet_port = 0;
		NetworkSocket::port_type PROPERTY_ACCESS(peer_port, protected, 0);
		int32_t _queue_count = 0;
		IpAddress _packet_ip;
		IpAddress PROPERTY_ACCESS(peer_ip, protected);

		std::shared_ptr<NetworkSocket> _socket;
		UdpServer* _server = nullptr;
		RingBuffer<uint8_t> _ring_buffer;

		std::array<uint8_t, PACKET_BUFFER_SIZE> _receive_buffer;
		std::array<uint8_t, PACKET_BUFFER_SIZE> _packet_buffer;

		friend struct UdpServer;
		friend struct ReliableUdpServer;
		void _connect_shared_socket( //
			std::shared_ptr<NetworkSocket> p_sock, IpAddress p_ip, NetworkSocket::port_type p_port, UdpServer* p_server
		);
		void _disconnect_shared_socket();
	};
}
