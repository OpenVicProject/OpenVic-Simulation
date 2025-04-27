#pragma once

#include <memory>

#include "openvic-simulation/multiplayer/IpAddress.hpp"
#include "openvic-simulation/multiplayer/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/PacketClient.hpp"
#include "openvic-simulation/multiplayer/PacketServer.hpp"
#include "openvic-simulation/multiplayer/StreamPacketClient.hpp"

namespace OpenVic {
	struct TcpServer : PacketServer {
		static constexpr size_t MAX_PENDING_CONNECTIONS = 8;

		bool is_connection_available() const override;
		NetworkError listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr = "*") override;
		std::unique_ptr<PacketClient> take_next_client_virtual() override;

		std::unique_ptr<TcpPacketStream> take_next_client();
	};
}
