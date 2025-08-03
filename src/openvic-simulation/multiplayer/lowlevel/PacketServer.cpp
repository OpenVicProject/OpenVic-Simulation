#include "PacketServer.hpp"

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

PacketServer::PacketServer() : _socket { memory::make_shared<NetworkSocket>() } {}

PacketServer::~PacketServer() {
	close();
}

NetworkSocket::port_type PacketServer::get_listening_port() const {
	NetworkSocket::port_type local_port;
	_socket->get_socket_address(nullptr, &local_port);
	return local_port;
}

bool PacketServer::is_connection_available() const {
	OV_ERR_FAIL_COND_V(_socket == nullptr, false);
	return _socket->is_open();
}

bool PacketServer::is_listening() const {
	OV_ERR_FAIL_COND_V(_socket == nullptr, false);
	return _socket->is_open();
}

NetworkError PacketServer::listen_to(NetworkSocket::port_type port, IpAddress const& bind_addr) {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(_socket->is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(!bind_addr.is_valid() && !bind_addr.is_wildcard(), NetworkError::INVALID_PARAMETER);

	NetworkError err;
	NetworkResolver::Type ip_type = NetworkResolver::Type::ANY;

	if (bind_addr.is_valid()) {
		ip_type = bind_addr.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	}

	err = _socket->open(NetworkSocket::Type::UDP, ip_type);

	if (err != NetworkError::OK) {
		return err;
	}

	_socket->set_blocking_enabled(false);
	_socket->set_reuse_address_enabled(true);
	err = _socket->bind(bind_addr, port);

	if (err != NetworkError::OK) {
		close();
		return err;
	}
	return NetworkError::OK;
}

void PacketServer::close() {
	if (_socket != nullptr) {
		_socket->close();
	}
}

NetworkError PacketServer::poll() {
	return NetworkError::OK;
}

memory::unique_base_ptr<PacketClient> PacketServer::take_next_client() {
	return memory::unique_base_ptr<PacketClient> { static_cast<PacketClient*>(_take_next_client()) };
}
