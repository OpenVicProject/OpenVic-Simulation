#pragma once

#include <memory>

#include "openvic-simulation/multiplayer/IpAddress.hpp"
#include "openvic-simulation/multiplayer/NetworkError.hpp"
#include "openvic-simulation/multiplayer/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/PacketClient.hpp"

namespace OpenVic {
	struct PacketServer {
		PacketServer();
		~PacketServer();

		NetworkSocket::port_type get_listening_port() const;
		bool is_listening() const;
		virtual bool is_connection_available() const;
		virtual NetworkError listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr = "*");
		virtual void close();
		virtual std::unique_ptr<PacketClient> take_next_client_virtual() = 0;
		virtual NetworkError poll();

	protected:
		std::shared_ptr<NetworkSocket> _socket;
	};
}
