
#include "TcpServer.hpp"

#include <memory>

#include "openvic-simulation/multiplayer/IpAddress.hpp"
#include "openvic-simulation/multiplayer/NetworkError.hpp"
#include "openvic-simulation/multiplayer/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/PacketClient.hpp"
#include "openvic-simulation/multiplayer/PacketServer.hpp"
#include "openvic-simulation/multiplayer/TcpPacketStream.hpp"

using namespace OpenVic;

bool TcpServer::is_connection_available() const {
	if (!PacketServer::is_connection_available()) {
		return false;
	}
	return _socket->poll(NetworkSocket::PollType::IN, 0) == NetworkError::OK;
}

NetworkError TcpServer::listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr) {
	NetworkError err;
	if (err = PacketServer::listen_to(port, bind_addr); err != NetworkError::OK) {
		return err;
	}

	err = _socket->listen(MAX_PENDING_CONNECTIONS);
	if (err != NetworkError::OK) {
		_socket->close();
	}
	return err;
}

std::unique_ptr<PacketClient> TcpServer::take_next_client_virtual() {
	std::unique_ptr<TcpStreamPacketClient> client = std::make_unique<TcpStreamPacketClient>();
	client->set_packet_stream(std::shared_ptr<TcpPacketStream> { take_next_client().release() });
	return client;
}

std::unique_ptr<TcpPacketStream> TcpServer::take_next_client() {
	if (!is_connection_available()) {
		return nullptr;
	}

	IpAddress ip;
	NetworkSocket::port_type port = 0;
	std::shared_ptr<NetworkSocket> ns = std::make_shared<NetworkSocket>(_socket->accept(ip, port));
	if (!ns->is_open()) {
		return nullptr;
	}

	std::unique_ptr<TcpPacketStream> result = std::make_unique<TcpPacketStream>();
	result->accept_socket(ns, ip, port);
	return result;
}
