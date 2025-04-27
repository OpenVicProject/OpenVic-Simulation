#pragma once

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/StreamPacketClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct TcpServer : PacketServer {
		static constexpr size_t MAX_PENDING_CONNECTIONS = 8;

		bool is_connection_available() const override;
		NetworkError listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr = "*") override;

		memory::unique_ptr<TcpPacketStream> take_packet_stream();

	protected:
		TcpStreamPacketClient* _take_next_client() override;
	};
}
