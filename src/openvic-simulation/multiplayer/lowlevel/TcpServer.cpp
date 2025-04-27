
#include "TcpServer.hpp"

#include <memory>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/StreamPacketClient.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

bool TcpServer::is_connection_available() const {
	if (!PacketServer::is_connection_available()) {
		return false;
	}
	return _socket->poll(NetworkSocket::PollType::IN, 0) == NetworkError::OK;
}

NetworkError TcpServer::listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr) {
	OV_ERR_FAIL_COND_V(!_socket, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(_socket->is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(!bind_addr.is_valid() && !bind_addr.is_wildcard(), NetworkError::INVALID_PARAMETER);

	NetworkError err;
	NetworkResolver::Type ip_type = NetworkResolver::Type::ANY;

	// If the bind address is valid use its type as the socket type
	if (bind_addr.is_valid()) {
		ip_type = bind_addr.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	}

	err = _socket->open(NetworkSocket::Type::TCP, ip_type);

	OV_ERR_FAIL_COND_V(err != NetworkError::OK, err);

	_socket->set_blocking_enabled(false);
	_socket->set_reuse_address_enabled(true);

	err = _socket->bind(bind_addr, port);

	if (err != NetworkError::OK) {
		_socket->close();
		return err;
	}

	err = _socket->listen(MAX_PENDING_CONNECTIONS);
	if (err != NetworkError::OK) {
		_socket->close();
	}
	return err;
}

memory::unique_ptr<TcpPacketStream> TcpServer::take_packet_stream() {
	if (!is_connection_available()) {
		return nullptr;
	}

	IpAddress ip;
	NetworkSocket::port_type port = 0;
	std::shared_ptr<NetworkSocket> ns = _socket->accept_as<NetworkSocket>(ip, port);
	if (!ns->is_open()) {
		return nullptr;
	}

	memory::unique_ptr<TcpPacketStream> result = memory::make_unique<TcpPacketStream>();
	result->accept_socket(ns, ip, port);
	return result;
}

TcpStreamPacketClient* TcpServer::_take_next_client() {
	memory::unique_ptr<TcpStreamPacketClient> client = memory::make_unique<TcpStreamPacketClient>();
	client->set_packet_stream(std::shared_ptr<TcpPacketStream> { take_packet_stream().release() });
	return client.release();
}
