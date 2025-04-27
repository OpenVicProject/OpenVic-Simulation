#pragma once

#include <concepts>
#include <memory>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct PacketServer {
		PacketServer();
		~PacketServer();

		NetworkSocket::port_type get_listening_port() const;
		bool is_listening() const;
		virtual bool is_connection_available() const;
		virtual NetworkError listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr = "*");
		virtual void close();
		virtual NetworkError poll();

		memory::unique_base_ptr<PacketClient> take_next_client();

		template<std::derived_from<PacketClient> T>
		memory::unique_ptr<T> take_next_client_as() {
			return memory::unique_ptr<T> { static_cast<T*>(_take_next_client()) };
		}

	protected:
		virtual PacketClient* _take_next_client() = 0;

		std::shared_ptr<NetworkSocket> _socket;
	};
}
