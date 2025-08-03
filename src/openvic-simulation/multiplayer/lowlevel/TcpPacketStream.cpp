
#include "TcpPacketStream.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocket.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

TcpPacketStream::TcpPacketStream() : _socket { std::make_shared<NetworkSocket>() } {}

TcpPacketStream::~TcpPacketStream() {
	close();
}

void TcpPacketStream::accept_socket(std::shared_ptr<NetworkSocket> p_sock, IpAddress p_host, NetworkSocket::port_type p_port) {
	_socket = p_sock;
	_socket->set_blocking_enabled(false);

	_timeout = GameManager::get_elapsed_milliseconds() + _timeout_seconds * 1000;
	_status = Status::CONNECTED;

	_client_address = p_host;
	_client_port = p_port;
}

int64_t TcpPacketStream::available_bytes() const {
	OV_ERR_FAIL_COND_V(_socket == nullptr, -1);
	return _socket->available_bytes();
}

NetworkError TcpPacketStream::bind(NetworkSocket::port_type port, IpAddress const& address) {
	OV_ERR_FAIL_COND_V(_socket != nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(_socket->is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V_MSG(
		port < 0 || port > 65535, NetworkError::INVALID_PARAMETER,
		"The local port number must be between 0 and 65535 (inclusive)."
	);

	NetworkResolver::Type ip_type = address.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	if (address.is_wildcard()) {
		ip_type = NetworkResolver::Type::ANY;
	}
	NetworkError err = _socket->open(NetworkSocket::Type::TCP, ip_type);
	if (err != NetworkError::OK) {
		return err;
	}
	_socket->set_blocking_enabled(false);
	return _socket->bind(address, port);
}

NetworkError TcpPacketStream::connect_to(IpAddress const& host, NetworkSocket::port_type port) {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);
	OV_ERR_FAIL_COND_V(_status != Status::NONE, NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(!host.is_valid(), NetworkError::INVALID_PARAMETER);
	OV_ERR_FAIL_COND_V_MSG(
		port < 1 || port > 65535, NetworkError::INVALID_PARAMETER,
		"The remote port number must be between 1 and 65535 (inclusive)."
	);

	if (!_socket->is_open()) {
		NetworkResolver::Type ip_type = host.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
		NetworkError err = _socket->open(NetworkSocket::Type::TCP, ip_type);
		if (err != NetworkError::OK) {
			return err;
		}
		_socket->set_blocking_enabled(false);
	}

	_timeout = GameManager::get_elapsed_milliseconds() + _timeout_seconds * 1000;
	NetworkError err = _socket->connect_to_host(host, port);

	if (err == NetworkError::OK) {
		_status = Status::CONNECTED;
	} else if (err == NetworkError::BUSY) {
		_status = Status::CONNECTING;
	} else {
		Logger::error("Connection to remote host failed!");
		close();
		return err;
	}

	_client_address = host;
	_client_port = port;

	return NetworkError::OK;
}

NetworkError TcpPacketStream::poll() {
	switch (_status) {
		using enum Status;
	case CONNECTED: {
		NetworkError err;
		err = _socket->poll(NetworkSocket::PollType::IN, 0);
		if (err == NetworkError::OK) {
			// FIN received
			if (_socket->available_bytes() == 0) {
				close();
				return NetworkError::OK;
			}
		}
		// Also poll write
		err = _socket->poll(NetworkSocket::PollType::IN_OUT, 0);
		if (err != NetworkError::OK && err != NetworkError::BUSY) {
			// Got an error
			close();
			_status = ERROR;
			return err;
		}
		return NetworkError::OK;
	}
	case CONNECTING: break;
	default:		 return NetworkError::OK;
	}

	NetworkError err = _socket->connect_to_host(_client_address, _client_port);

	if (err == NetworkError::OK) {
		_status = Status::CONNECTED;
		return NetworkError::OK;
	} else if (err == NetworkError::BUSY) {
		// Check for connect timeout
		if (GameManager::get_elapsed_milliseconds() > _timeout) {
			close();
			_status = Status::ERROR;
			return err;
		}
		// Still trying to connect
		return NetworkError::OK;
	}

	close();
	_status = Status::ERROR;
	return err;
}

NetworkError TcpPacketStream::wait(NetworkSocket::PollType p_type, int p_timeout) {
	OV_ERR_FAIL_COND_V(_socket == nullptr || !_socket->is_open(), NetworkError::UNAVAILABLE);
	return _socket->poll(p_type, p_timeout);
}

void TcpPacketStream::close() {
	if (_socket != nullptr && _socket->is_open()) {
		_socket->close();
	}

	_timeout = 0;
	_status = Status::NONE;
	_client_address = {};
	_client_port = 0;
}

NetworkSocket::port_type TcpPacketStream::get_bound_port() const {
	NetworkSocket::port_type local_port;
	_socket->get_socket_address(nullptr, &local_port);
	return local_port;
}

bool TcpPacketStream::is_connected() const {
	return _status == Status::CONNECTED;
}

IpAddress TcpPacketStream::get_connected_address() const {
	return _client_address;
}

NetworkSocket::port_type TcpPacketStream::get_connected_port() const {
	return _client_port;
}

TcpPacketStream::Status TcpPacketStream::get_status() const {
	return _status;
}

void TcpPacketStream::set_timeout_seconds(uint64_t timeout) {
	_timeout_seconds = timeout;
}

uint64_t TcpPacketStream::get_timeout_seconds() const {
	return _timeout_seconds;
}

void TcpPacketStream::set_no_delay(bool p_enabled) {
	OV_ERR_FAIL_COND(_socket == nullptr || !_socket->is_open());
	_socket->set_tcp_no_delay_enabled(p_enabled);
}

NetworkError TcpPacketStream::set_data(std::span<const uint8_t> buffer) {
	size_t _ = 0;
	return _set_data<true>(buffer, _);
}

NetworkError TcpPacketStream::set_data_no_blocking(std::span<const uint8_t> buffer, size_t& r_sent) {
	return _set_data<false>(buffer, r_sent);
}

NetworkError TcpPacketStream::get_data(std::span<uint8_t> buffer_to_set) {
	return _get_data<true>(buffer_to_set);
}

NetworkError TcpPacketStream::get_data_no_blocking(std::span<uint8_t> buffer_to_set, size_t& r_received) {
	NetworkError result = _get_data<false>(buffer_to_set);
	r_received = buffer_to_set.size();
	return result;
}

template<bool IsBlocking>
NetworkError TcpPacketStream::_set_data(std::span<const uint8_t> buffer, size_t& r_sent) {
	OV_ERR_FAIL_COND_V(_socket == nullptr, NetworkError::UNAVAILABLE);

	if (_status != Status::CONNECTED) {
		return NetworkError::FAILED;
	}

	NetworkError err;
	int data_to_send = buffer.size_bytes();
	const uint8_t* offset = buffer.data();
	int64_t total_sent = 0;

	while (data_to_send) {
		int64_t sent_amount = 0;
		err = _socket->send(offset, data_to_send, sent_amount);

		if (err != NetworkError::OK) {
			if (err != NetworkError::BUSY) {
				close();
				return err;
			}

			if constexpr (!IsBlocking) {
				r_sent = total_sent;
				return NetworkError::OK;
			} else {
				// Block and wait for the socket to accept more data
				err = _socket->poll(NetworkSocket::PollType::OUT, -1);
				if (err != NetworkError::OK) {
					close();
					return err;
				}
			}
		} else {
			data_to_send -= sent_amount;
			offset += sent_amount;
			total_sent += sent_amount;
		}
	}

	r_sent = total_sent;

	return NetworkError::OK;
}

template NetworkError TcpPacketStream::_set_data<true>(std::span<const uint8_t> buffer, size_t& r_sent);
template NetworkError TcpPacketStream::_set_data<false>(std::span<const uint8_t> buffer, size_t& r_sent);

template<bool IsBlocking>
NetworkError TcpPacketStream::_get_data(std::span<uint8_t>& r_buffer) {
	if (_status != Status::CONNECTED) {
		return NetworkError::FAILED;
	}

	NetworkError err;
	int to_read = r_buffer.size_bytes();
	int total_read = 0;

	while (to_read) {
		int64_t read = 0;
		err = _socket->receive(r_buffer.data() + total_read, to_read, read);

		if (err != NetworkError::OK) {
			if (err != NetworkError::BUSY) {
				close();
				return err;
			}

			if constexpr (!IsBlocking) {
				r_buffer = { r_buffer.data(), static_cast<size_t>(total_read) };
				return NetworkError::OK;
			} else {
				err = _socket->poll(NetworkSocket::PollType::IN, -1);

				if (err != NetworkError::OK) {
					close();
					return err;
				}
			}
		} else if (read == 0) {
			close();
			r_buffer = { r_buffer.data(), static_cast<size_t>(total_read) };
			return NetworkError::EMPTY_BUFFER;
		} else {
			to_read -= read;
			total_read += read;

			if constexpr (!IsBlocking) {
				r_buffer = { r_buffer.data(), static_cast<size_t>(total_read) };
				return NetworkError::OK;
			}
		}
	}

	r_buffer = { r_buffer.data(), static_cast<size_t>(total_read) };

	return NetworkError::OK;
}

template NetworkError TcpPacketStream::_get_data<true>(std::span<uint8_t>& r_buffer);
template NetworkError TcpPacketStream::_get_data<false>(std::span<uint8_t>& r_buffer);
