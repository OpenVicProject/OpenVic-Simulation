
#include "UdpServer.hpp"

#include <cstdint>

#include <range/v3/algorithm/find.hpp>

#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketServer.hpp"
#include "openvic-simulation/multiplayer/lowlevel/UdpClient.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

bool UdpServer::is_connection_available() const {
	if (!PacketServer::is_connection_available()) {
		return false;
	}
	return _pending_clients.size() > 0;
}

void UdpServer::close() {
	PacketServer::close();
	for (ClientRef<false>& client : _clients) {
		client.client->_disconnect_shared_socket();
	}
	for (ClientRef<true>& client : _pending_clients) {
		client.client->_disconnect_shared_socket();
	}
	_clients.clear();
	_pending_clients.clear();
}

UdpClient* UdpServer::_take_next_client() {
	if (!is_connection_available()) {
		return nullptr;
	};

	ClientRef<true>& client = _pending_clients.front();
	_clients.emplace_back(client.client.release(), client.ip, client.port);
	_pending_clients.pop_front();
	return _clients.back().client;
}

UdpClient* UdpServer::_create_client() {
	return new UdpClient();
}

void UdpServer::remove_client(IpAddress ip, NetworkSocket::port_type port) {
	const ClientRef<false> client { nullptr, ip, port };
	if (memory::vector<ClientRef<false>>::iterator it = ranges::find(_clients, client); it != _clients.end()) {
		_clients.erase(it);
	}
}

NetworkError UdpServer::poll() {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(!_socket->is_open(), NetworkError::UNCONFIGURED);

	NetworkError err;
	int64_t read;
	IpAddress ip;
	NetworkSocket::port_type port;
	while (true) {
		err = _socket->receive_from(_receive_buffer.data(), sizeof(_receive_buffer), read, ip, port);
		if (err != NetworkError::OK) {
			if (err == NetworkError::BUSY) {
				break;
			}
			return err;
		}

		ClientRef<false> p;
		p.ip = ip;
		p.port = port;
		UdpClient* client_ptr = nullptr;
		if (decltype(_clients)::iterator it = ranges::find(_clients, p); it != _clients.end()) {
			client_ptr = it->client;
		} else if (decltype(_pending_clients)::iterator pend_it = ranges::find(_pending_clients, ClientRef<true> { p });
				   pend_it != _pending_clients.end()) {
			client_ptr = pend_it->client.get();
		}

		if (client_ptr) {
			client_ptr->store_packet(ip, port, std::span<uint8_t> { _receive_buffer.data(), static_cast<size_t>(read) });
		} else {
			if (_pending_clients.size() >= _max_pending_clients) {
				// Drop connection.
				continue;
			}
			// It's a new client, add it to the pending list.
			ClientRef<true> ref { memory::unique_ptr<UdpClient>(_create_client()), ip, port };
			ref.client->_connect_shared_socket(_socket, ip, port, this);
			ref.client->store_packet(ip, port, std::span<uint8_t> { _receive_buffer.data(), static_cast<size_t>(read) });
			_pending_clients.push_back(std::move(ref));
		}
	}
	return NetworkError::OK;
}

void UdpServer::set_max_pending_clients(uint32_t clients) {
	OV_ERR_FAIL_COND_MSG(
		clients < 0, "Max pending connections value must be a positive number (0 means refuse new connections)."
	);
	_max_pending_clients = clients;
	if (clients > _pending_clients.size()) {
		_pending_clients.erase(_pending_clients.begin() + clients + 1, _pending_clients.end());
	}
}

uint32_t UdpServer::get_max_pending_clients() const {
	return _max_pending_clients;
}
