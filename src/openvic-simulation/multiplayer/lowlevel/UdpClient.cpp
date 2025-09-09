#include "UdpClient.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpServer.hpp"

using namespace OpenVic;

UdpClient::UdpClient() : _socket(std::make_shared<NetworkSocket>()), _ring_buffer(16) {} // NOLINT

UdpClient::~UdpClient() {
	close();
}

NetworkError UdpClient::bind(NetworkSocket::port_type port, IpAddress const& address, size_t min_buffer_size) {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(_socket->is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(!address.is_valid() && !address.is_wildcard(), NetworkError::INVALID_PARAMETER);
	OV_ERR_FAIL_COND_V_MSG(
		port > 65535, NetworkError::INVALID_PARAMETER, "The local port number must be between 0 and 65535 (inclusive)."
	);

	NetworkError err;
	NetworkResolver::Type ip_type = NetworkResolver::Type::ANY;

	if (address.is_valid()) {
		ip_type = address.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	}

	err = _socket->open(NetworkSocket::Type::UDP, ip_type);

	if (err != NetworkError::OK) {
		return err;
	}

	_socket->set_blocking_enabled(false);
	_socket->set_broadcasting_enabled(_is_broadcasting);
	err = _socket->bind(address, port);

	if (err != NetworkError::OK) {
		_socket->close();
		return err;
	}
	if (min_buffer_size < _ring_buffer.capacity()) {
		_ring_buffer.shrink_to_fit();
	}
	_ring_buffer.reserve_power(std::bit_ceil(min_buffer_size));
	return NetworkError::OK;
}

void UdpClient::close() {
	if (_server) {
		_server->remove_client(peer_ip, peer_port);
		_server = nullptr;
		_socket = std::make_shared<NetworkSocket>();
	} else if (_socket) {
		_socket->close();
	}
	_ring_buffer.reserve_power(16);
	_queue_count = 0;
	_is_connected = false;
}

NetworkError UdpClient::connect_to(IpAddress const& host, NetworkSocket::port_type port) {
	OV_ERR_FAIL_COND_V(_server, NetworkError::LOCKED);
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!host.is_valid(), NetworkError::INVALID_PARAMETER);

	NetworkError err;

	if (!_socket->is_open()) {
		NetworkResolver::Type ip_type = host.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
		err = _socket->open(NetworkSocket::Type::UDP, ip_type);
		OV_ERR_FAIL_COND_V(err != NetworkError::OK, err);
		_socket->set_blocking_enabled(false);
	}

	err = _socket->connect_to_host(host, port);

	// I see no reason why we should get ERR_BUSY (wouldblock/eagain) here.
	// This is UDP, so connect is only used to tell the OS to which socket
	// it should deliver packets when multiple are bound on the same address/port.
	if (err != NetworkError::OK) {
		close();
		OV_ERR_FAIL_V_MSG(err, "Unable to connect");
	}

	_is_connected = true;

	peer_ip = host;
	peer_port = port;

	// Flush any packet we might still have in queue.
	_ring_buffer.clear();
	return NetworkError::OK;
}

NetworkSocket::port_type UdpClient::get_bound_port() const {
	NetworkSocket::port_type local_port;
	_socket->get_socket_address(nullptr, &local_port);
	return local_port;
}

IpAddress UdpClient::get_last_packet_ip() const {
	return _packet_ip;
}

NetworkSocket::port_type UdpClient::get_last_packet_port() const {
	return _packet_port;
}

bool UdpClient::is_bound() const {
	return _socket != nullptr && _socket->is_open();
}

bool UdpClient::is_connected() const {
	return _is_connected;
}

NetworkError UdpClient::wait() {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	return _socket->poll(NetworkSocket::PollType::IN, -1);
}

NetworkError UdpClient::set_destination(HostnameAddress const& addr, NetworkSocket::port_type port) {
	OV_ERR_FAIL_COND_V_MSG(
		_is_connected, NetworkError::UNCONFIGURED, "Destination address cannot be set for connected sockets"
	);
	peer_ip = addr.resolved_address();
	peer_port = port;
	return NetworkError::OK;
}

NetworkError UdpClient::store_packet(IpAddress p_ip, NetworkSocket::port_type p_port, std::span<uint8_t> p_buf) {
	uint16_t buffer_size = p_buf.size();
	std::span<const uint8_t, 16> ipv6 = p_ip.get_ipv6();

	if (_ring_buffer.capacity() - _ring_buffer.size() <
		buffer_size + ipv6.size_bytes() + sizeof(p_port) + sizeof(buffer_size)) {
		return NetworkError::OUT_OF_MEMORY;
	}
	_ring_buffer.append_range(ipv6);
	_ring_buffer.append((uint8_t*)&p_port, sizeof(p_port));
	_ring_buffer.append((uint8_t*)&buffer_size, sizeof(buffer_size));
	_ring_buffer.append_range(p_buf);
	++_queue_count;
	return NetworkError::OK;
}

NetworkError UdpClient::_set_packet(std::span<uint8_t> buffer) {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(!peer_ip.is_valid(), NetworkError::UNCONFIGURED);

	NetworkError err;
	int64_t sent = -1;

	if (!_socket->is_open()) {
		NetworkResolver::Type ip_type = peer_ip.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
		err = _socket->open(NetworkSocket::Type::UDP, ip_type);
		OV_ERR_FAIL_COND_V(err != NetworkError::OK, err);
		_socket->set_blocking_enabled(false);
		_socket->set_broadcasting_enabled(_is_broadcasting);
	}

	do {
		if (_is_connected && !_server) {
			err = _socket->send(buffer.data(), buffer.size(), sent);
		} else {
			err = _socket->send_to(buffer.data(), buffer.size(), sent, peer_ip, peer_port);
		}
		if (err != NetworkError::OK) {
			if (err != NetworkError::BUSY) {
				return err;
			} else if (!_is_blocking) {
				return NetworkError::BUSY;
			}
			// Keep trying to send full packet
			continue;
		}
		return NetworkError::OK;

	} while (sent != buffer.size());

	return NetworkError::OK;
}

NetworkError UdpClient::_get_packet(std::span<uint8_t>& r_buffer) {
	NetworkError err = _poll();
	if (err != NetworkError::OK) {
		return err;
	}
	if (_queue_count == 0) {
		return NetworkError::UNAVAILABLE;
	}

	uint16_t size = 0;
	std::array<uint8_t, 16> ipv6 {};
	_ring_buffer.read_buffer_to(ipv6.data(), sizeof(ipv6));
	_packet_ip.set_ipv6(ipv6);
	_ring_buffer.read_buffer_to((uint8_t*)&_packet_port, sizeof(_packet_port));
	_ring_buffer.read_buffer_to((uint8_t*)&size, sizeof(size));
	_ring_buffer.read_buffer_to(_packet_buffer.data(), size);
	--_queue_count;
	r_buffer = { _packet_buffer.data(), size };

	return NetworkError::OK;
}

NetworkError UdpClient::join_multicast_group(IpAddress addr, std::string_view interface_name) {
	OV_ERR_FAIL_COND_V(_server, NetworkError::LOCKED);
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(!addr.is_valid(), NetworkError::INVALID_PARAMETER);

	if (!_socket->is_open()) {
		NetworkResolver::Type ip_type = addr.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
		NetworkError err = _socket->open(NetworkSocket::Type::UDP, ip_type);
		OV_ERR_FAIL_COND_V(err != NetworkError::OK, err);
		_socket->set_blocking_enabled(false);
		_socket->set_broadcasting_enabled(_is_broadcasting);
	}
	return _socket->change_multicast_group(addr, interface_name, true);
}

NetworkError UdpClient::leave_multicast_group(IpAddress addr, std::string_view interface_name) {
	OV_ERR_FAIL_COND_V(_server, NetworkError::LOCKED);
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(!_socket->is_open(), NetworkError::UNCONFIGURED);
	return _socket->change_multicast_group(addr, interface_name, false);
}

bool UdpClient::is_broadcast_enabled() const {
	return _is_broadcasting;
}

void UdpClient::set_broadcast_enabled(bool enabled) {
	OV_ERR_FAIL_COND(_server);
	_is_broadcasting = enabled;
	if (_socket && _socket->is_open()) {
		_socket->set_broadcasting_enabled(enabled);
	}
}

bool UdpClient::is_blocking() const {
	return _is_blocking;
}

void UdpClient::set_blocking(bool blocking) {
	_is_blocking = blocking;
}

int64_t UdpClient::available_packets() const {
	NetworkError err = const_cast<UdpClient*>(this)->_poll();
	if (err != NetworkError::OK) {
		return -1;
	}

	return _queue_count;
}

int64_t UdpClient::max_packet_size() const {
	return MAX_PACKET_SIZE;
}

NetworkError UdpClient::_poll() {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);

	if (!_socket->is_open()) {
		return NetworkError::FAILED;
	}
	if (_server) {
		return NetworkError::OK; // Handled by UDPServer.
	}

	NetworkError err;
	int64_t read;
	IpAddress ip;
	NetworkSocket::port_type port;

	while (true) {
		if (_is_connected) {
			err = _socket->receive(_receive_buffer.data(), sizeof(_receive_buffer), read);
			ip = peer_ip;
			port = peer_port;
		} else {
			err = _socket->receive_from(_receive_buffer.data(), sizeof(_receive_buffer), read, ip, port);
		}

		if (err != NetworkError::OK) {
			if (err == NetworkError::BUSY) {
				break;
			}
			return err;
		}

		err = store_packet(ip, port, { _receive_buffer.data(), static_cast<size_t>(read) });
#ifdef TOOLS_ENABLED
		if (err != NetworkError::OK) {
			Logger::warning("Buffer full, dropping packets!");
		}
#endif
	}

	return NetworkError::OK;
}

void UdpClient::_connect_shared_socket( //
	std::shared_ptr<NetworkSocket> p_sock, IpAddress p_ip, NetworkSocket::port_type p_port, UdpServer* p_server
) {
	_server = p_server;
	_is_connected = true;
	_socket = std::move(p_sock);
	peer_ip = p_ip;
	peer_port = p_port;
	_packet_ip = peer_ip;
	_packet_port = peer_port;
}

void UdpClient::_disconnect_shared_socket() {
	_server = nullptr;
	_socket = std::make_shared<NetworkSocket>();
	close();
}
