#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Deque.hpp"

namespace OpenVic {
	struct UdpServer : PacketServer {
		bool is_connection_available() const override;
		void close() override;
		NetworkError poll() override;

		void remove_client(IpAddress ip, NetworkSocket::port_type port);

		void set_max_pending_clients(uint32_t clients);
		uint32_t get_max_pending_clients() const;

	protected:
		UdpClient* _take_next_client() override;
		virtual UdpClient* _create_client();

		template<bool Owner>
		struct ClientRef {
			static constexpr std::bool_constant<Owner> is_owner {};

			std::conditional_t<is_owner, memory::unique_ptr<UdpClient>, UdpClient*> client = nullptr;
			IpAddress ip;
			NetworkSocket::port_type port = 0;

			ClientRef() = default;
			ClientRef(decltype(client)&& client, IpAddress ip, NetworkSocket::port_type port = 0)
				: client { std::move(client) }, ip { ip }, port { port } {}
			ClientRef(ClientRef<!is_owner> const& ref) : client { ref.client }, ip { ref.ip }, port { ref.port } {}
			ClientRef(ClientRef const&) = default;
			ClientRef& operator=(ClientRef const&) = default;
			ClientRef(ClientRef&&) = default;
			ClientRef& operator=(ClientRef&&) = default;

			template<bool Owner2>
			bool operator==(ClientRef<Owner2> const& p_other) const {
				return (ip == p_other.ip && port == p_other.port);
			}
		};

		uint32_t _max_pending_clients = 16;

		memory::vector<ClientRef<false>> _clients;
		OpenVic::utility::deque<ClientRef<true>> _pending_clients;

		std::array<uint8_t, UdpClient::PACKET_BUFFER_SIZE> _receive_buffer;
	};
}
