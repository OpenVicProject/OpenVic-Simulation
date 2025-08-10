#ifdef _WIN32
#include "WindowsSocket.hpp"

#include <bit>
#include <cerrno>
#include <charconv>
#include <memory>
#include <span>
#include <system_error>
#include <utility>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#include <winsock2.h>
//
#include <mswsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

// Thank You Microsoft
#define WIN_SOCKET_ERROR (SOCKET)(~0)
#undef SOCKET_ERROR

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocketBase.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

// Workaround missing flag in MinGW
#if defined(__MINGW32__) && !defined(SIO_UDP_NETRESET)
#define SIO_UDP_NETRESET _WSAIOW(IOC_VENDOR, 15)
#endif

using namespace OpenVic;

WindowsSocket::WindowsSocket() {}

WindowsSocket::~WindowsSocket() {
	close();
}

NetworkError WindowsSocket::_get_socket_error() const {
	int err = WSAGetLastError();
	switch (err) {
	case WSAEISCONN:	   return NetworkError::IS_CONNECTED;
	case WSAEINPROGRESS:
	case WSAEALREADY:	   return NetworkError::IN_PROGRESS;
	case WSAEWOULDBLOCK:   return NetworkError::WOULD_BLOCK;
	case WSAEADDRINUSE:
	case WSAEADDRNOTAVAIL: return NetworkError::ADDRESS_INVALID_OR_UNAVAILABLE;
	case WSAEACCES:		   return NetworkError::UNAUTHORIZED;
	case WSAEMSGSIZE:
	case WSAENOBUFS:	   return NetworkError::BUFFER_TOO_SMALL;
	default: //
		Logger::info("Socket error: ", strerror(errno));
		return NetworkError::OTHER;
	}
}

void WindowsSocket::_set_ip_port(sockaddr_storage* addr, IpAddress* r_ip, port_type* r_port) {
	if (addr->ss_family == AF_INET) {
		struct sockaddr_in* addr4 = (struct sockaddr_in*)addr;
		if (r_ip) {
			r_ip->set_ipv4(std::bit_cast<std::span<uint8_t, 4>>(&addr4->sin_addr.s_addr));
		}
		if (r_port) {
			*r_port = ntohs(addr4->sin_port);
		}
	} else if (addr->ss_family == AF_INET6) {
		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
		if (r_ip) {
			r_ip->set_ipv6(addr6->sin6_addr.s6_addr);
		}
		if (r_port) {
			*r_port = ntohs(addr6->sin6_port);
		}
	}
}

bool WindowsSocket::_can_use_ip(IpAddress const& p_ip, const bool p_for_bind) const {
	if (p_for_bind && !(p_ip.is_valid() || p_ip.is_wildcard())) {
		return false;
	} else if (!p_for_bind && !p_ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	NetworkResolver::Type type = p_ip.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	return !(_ip_type != NetworkResolver::Type::ANY && !p_ip.is_wildcard() && _ip_type != type);
}

void WindowsSocket::setup() {
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
}

void WindowsSocket::cleanup() {
	WSACleanup();
}

NetworkError WindowsSocket::open(Type sock_type, NetworkResolver::Type& ip_type) {
	OV_ERR_FAIL_COND_V(is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(
		ip_type > NetworkResolver::Type::ANY || ip_type < NetworkResolver::Type::NONE, NetworkError::INVALID_PARAMETER
	);

	int family = ip_type == NetworkResolver::Type::IPV4 ? AF_INET : AF_INET6;
	int protocol = sock_type == Type::TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int type = sock_type == Type::TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, type, protocol);

	if (_sock == INVALID_SOCKET && ip_type == NetworkResolver::Type::ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		ip_type = NetworkResolver::Type::IPV4;
		family = AF_INET;
		_sock = socket(family, type, protocol);
	}

	OV_ERR_FAIL_COND_V(_sock == INVALID_SOCKET, NetworkError::SOCKET_ERROR);
	_ip_type = ip_type;

	if (family == AF_INET6) {
		// Select IPv4 over IPv6 mapping.
		set_ipv6_only_enabled(ip_type != NetworkResolver::Type::ANY);
	}

	if (protocol == IPPROTO_UDP) {
		// Make sure to disable broadcasting for UDP sockets.
		// Depending on the OS, this option might or might not be enabled by default. Let's normalize it.
		set_broadcasting_enabled(false);
	}

	_is_stream = sock_type == Type::TCP;

	if (!_is_stream) {
		// Disable windows feature/bug reporting WSAECONNRESET/WSAENETRESET when
		// recv/recvfrom and an ICMP reply was received from a previous send/sendto.
		unsigned long disable = 0;
		if (ioctlsocket(_sock, SIO_UDP_CONNRESET, &disable) == -1) {
			Logger::info("Unable to turn off UDP WSAECONNRESET behavior on Windows.");
		}
		if (ioctlsocket(_sock, SIO_UDP_NETRESET, &disable) == -1) {
			// This feature seems not to be supported on wine.
			Logger::info("Unable to turn off UDP WSAENETRESET behavior on Windows.");
		}
	}
	return NetworkError::OK;
}

void WindowsSocket::close() {
	if (_sock != INVALID_SOCKET) {
		closesocket(_sock);
	}

	_sock = INVALID_SOCKET;
	_ip_type = NetworkResolver::Type::NONE;
	_is_stream = false;
}

NetworkError WindowsSocket::bind(IpAddress const& p_addr, port_type p_port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(p_addr, true), NetworkError::INVALID_PARAMETER);

	sockaddr_storage addr; // NOLINT
	size_t addr_size = _set_addr_storage(&addr, p_addr, p_port, _ip_type);

	if (::bind(_sock, (struct sockaddr*)&addr, addr_size) != 0) {
		NetworkError err = _get_socket_error();
		Logger::info("Failed to bind socket. Error: ", strerror(errno));
		close();
		return err;
	}

	return NetworkError::OK;
}

NetworkError WindowsSocket::listen(int p_max_pending) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	if (::listen(_sock, p_max_pending) != 0) {
		NetworkError err = _get_socket_error();
		Logger::info("Failed to listen from socket. Error: ", strerror(errno));
		close();
		return err;
	}

	return NetworkError::OK;
}

NetworkError WindowsSocket::connect_to_host(IpAddress const& p_addr, port_type p_port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(p_addr, true), NetworkError::INVALID_PARAMETER);

	struct sockaddr_storage addr; // NOLINT
	size_t addr_size = _set_addr_storage(&addr, p_addr, p_port, _ip_type);

	if (::WSAConnect(_sock, (struct sockaddr*)&addr, addr_size, nullptr, nullptr, nullptr, nullptr) != 0) {
		NetworkError err = _get_socket_error();

		switch (err) {
			using enum NetworkError;
		// We are already connected.
		case IS_CONNECTED: return NetworkError::OK;
		// Still waiting to connect, try again in a while.
		case WOULD_BLOCK:
		case IN_PROGRESS:  return NetworkError::BUSY;
		default:
			Logger::info("Connection to remote host failed. Error: ", strerror(errno));
			close();
			return err;
		}
	}

	return NetworkError::OK;
}

NetworkError WindowsSocket::poll(PollType p_type, int p_timeout) const {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	bool ready = false;
	fd_set rd, wr, ex;
	fd_set* rdp = nullptr;
	fd_set* wrp = nullptr;
	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_ZERO(&ex);
	FD_SET(_sock, &ex);
	struct timeval timeout = { p_timeout / 1000, (p_timeout % 1000) * 1000 };
	// For blocking operation, pass nullptr timeout pointer to select.
	struct timeval* tp = nullptr;
	if (p_timeout >= 0) {
		// If timeout is non-negative, we want to specify the timeout instead.
		tp = &timeout;
	}

// Windows loves its idiotic macros
#pragma push_macro("IN")
#undef IN
#pragma push_macro("OUT")
#undef OUT
	switch (p_type) {
		using enum PollType;
	case IN:
		FD_SET(_sock, &rd);
		rdp = &rd;
		break;
	case OUT:
		FD_SET(_sock, &wr);
		wrp = &wr;
		break;
	case IN_OUT:
		FD_SET(_sock, &rd);
		FD_SET(_sock, &wr);
		rdp = &rd;
		wrp = &wr;
	}
#pragma pop_macro("IN")
#pragma pop_macro("OUT")
	// WSAPoll is broken: https://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/.
	int ret = select(1, rdp, wrp, &ex, tp);

	if (ret == WIN_SOCKET_ERROR) {
		return NetworkError::SOCKET_ERROR;
	}

	if (ret == 0) {
		return NetworkError::BUSY;
	}

	if (FD_ISSET(_sock, &ex)) {
		NetworkError err = _get_socket_error();
		Logger::info("Exception when polling socket. Error: ", strerror(errno));
		return err;
	}

	if (rdp && FD_ISSET(_sock, rdp)) {
		ready = true;
	}
	if (wrp && FD_ISSET(_sock, wrp)) {
		ready = true;
	}

	return ready ? NetworkError::OK : NetworkError::BUSY;
}

NetworkError WindowsSocket::receive(uint8_t* p_buffer, size_t p_len, int64_t& r_read) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	r_read = ::recv(_sock, (char*)p_buffer, p_len, 0);

	if (r_read < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

NetworkError WindowsSocket::receive_from( //
	uint8_t* p_buffer, size_t p_len, int64_t& r_read, IpAddress& r_ip, port_type& r_port, bool p_peek
) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	struct sockaddr_storage from; // NOLINT
	socklen_t len = sizeof(from);
	std::memset(&from, 0, len);

	r_read = ::recvfrom(_sock, (char*)p_buffer, p_len, p_peek ? MSG_PEEK : 0, (struct sockaddr*)&from, &len);

	if (r_read < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	if (from.ss_family == AF_INET) {
		struct sockaddr_in* sin_from = (struct sockaddr_in*)&from;
		r_ip.set_ipv4(std::bit_cast<std::span<uint8_t, 4>>(&sin_from->sin_addr));
		r_port = ntohs(sin_from->sin_port);
	} else if (from.ss_family == AF_INET6) {
		struct sockaddr_in6* s6_from = (struct sockaddr_in6*)&from;
		r_ip.set_ipv6(std::bit_cast<std::span<uint8_t, 16>>(&s6_from->sin6_addr));
		r_port = ntohs(s6_from->sin6_port);
	} else {
		// Unsupported socket family, should never happen.
		OV_ERR_FAIL_V(NetworkError::UNSUPPORTED);
	}

	return NetworkError::OK;
}

NetworkError WindowsSocket::send(const uint8_t* p_buffer, size_t p_len, int64_t& r_sent) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	int flags = 0;
	r_sent = ::send(_sock, (const char*)p_buffer, p_len, flags);

	if (r_sent < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

NetworkError
WindowsSocket::send_to(const uint8_t* p_buffer, size_t p_len, int64_t& r_sent, IpAddress const& p_ip, port_type p_port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	struct sockaddr_storage addr; // NOLINT
	size_t addr_size = _set_addr_storage(&addr, p_ip, p_port, _ip_type);
	r_sent = ::sendto(_sock, (const char*)p_buffer, p_len, 0, (struct sockaddr*)&addr, addr_size);

	if (r_sent < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

WindowsSocket* WindowsSocket::_accept(IpAddress& r_ip, port_type& r_port) {
	OV_ERR_FAIL_COND_V(!is_open(), nullptr);

	struct sockaddr_storage their_addr; // NOLINT
	socklen_t size = sizeof(their_addr);
	SOCKET fd = ::accept(_sock, (struct sockaddr*)&their_addr, &size);
	if (fd == INVALID_SOCKET) {
		NetworkError err = _get_socket_error();
		Logger::info("Error when accepting socket connection. Error: ", strerror(errno));
		return nullptr;
	}

	_set_ip_port(&their_addr, &r_ip, &r_port);

	WindowsSocket* ns = new WindowsSocket();
	ns->_sock = fd;
	ns->_ip_type = _ip_type;
	ns->_is_stream = _is_stream;
	ns->set_blocking_enabled(false);
	return ns;
}

bool WindowsSocket::is_open() const {
	return _sock != INVALID_SOCKET;
}

int WindowsSocket::available_bytes() const {
	OV_ERR_FAIL_COND_V(!is_open(), -1);

	unsigned long len;
	int ret = ioctlsocket(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		Logger::info("Error when checking available bytes on socket. Error: ", strerror(errno));
		return -1;
	}
	return len;
}

NetworkError WindowsSocket::get_socket_address(IpAddress* r_ip, port_type* r_port) const {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::NOT_OPEN);

	struct sockaddr_storage saddr; // NOLINT
	socklen_t len = sizeof(saddr);
	if (getsockname(_sock, (struct sockaddr*)&saddr, &len) != 0) {
		NetworkError err = _get_socket_error();
		Logger::info("Error when reading local socket address. Error: ", strerror(errno));
		return err;
	}
	_set_ip_port(&saddr, r_ip, r_port);
	return NetworkError::OK;
}

NetworkError WindowsSocket::set_broadcasting_enabled(bool p_enabled) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == NetworkResolver::Type::IPV6) {
		return NetworkError::UNSUPPORTED;
	}

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&par, sizeof(int)) != 0) {
		Logger::warning("Unable to change broadcast setting.");
		return NetworkError::BROADCAST_CHANGE_FAILED;
	}
	return NetworkError::OK;
}

void WindowsSocket::set_blocking_enabled(bool p_enabled) {
	OV_ERR_FAIL_COND(!is_open());

	int ret = 0;
	unsigned long par = p_enabled ? 0 : 1;
	ret = ioctlsocket(_sock, FIONBIO, &par);
	if (ret != 0) {
		Logger::warning("Unable to change non-block mode.");
	}
}

void WindowsSocket::set_ipv6_only_enabled(bool p_enabled) {
	OV_ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	OV_ERR_FAIL_COND(_ip_type == NetworkResolver::Type::IPV4);

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&par, sizeof(int)) != 0) {
		Logger::warning("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void WindowsSocket::set_tcp_no_delay_enabled(bool p_enabled) {
	OV_ERR_FAIL_COND(!is_open());
	OV_ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&par, sizeof(int)) < 0) {
		Logger::warning("Unable to set TCP no delay option.");
	}
}

void WindowsSocket::set_reuse_address_enabled(bool p_enabled) {
	OV_ERR_FAIL_COND(!is_open());

	// On Windows, enabling SO_REUSEADDR actually would also enable reuse port, very bad on TCP. Denying...
	// Windows does not have this option, SO_REUSEADDR in this magical world means SO_REUSEPORT
}

NetworkError WindowsSocket::change_multicast_group(IpAddress const& p_multi_address, std::string_view p_if_name, bool p_add) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(p_multi_address, false), NetworkError::INVALID_PARAMETER);

	// Need to force level and af_family to IP(v4) when using dual stacking and provided multicast group is IPv4.
	NetworkResolver::Type type =
		_ip_type == NetworkResolver::Type::ANY && p_multi_address.is_ipv4() ? NetworkResolver::Type::IPV4 : _ip_type;
	// This needs to be the proper level for the multicast group, no matter if the socket is dual stacking.
	int level = type == NetworkResolver::Type::IPV4 ? IPPROTO_IP : IPPROTO_IPV6;
	int ret = -1;

	IpAddress if_ip;
	uint32_t if_v6id = 0;
	OpenVic::string_map_t<NetworkResolver::InterfaceInfo> if_info = NetworkResolver::singleton().get_local_interfaces();
	for (std::pair<memory::string, NetworkResolver::InterfaceInfo> const& pair : if_info) {
		NetworkResolver::InterfaceInfo const& c = pair.second;
		if (c.name != p_if_name) {
			continue;
		}

		std::from_chars_result from_chars =
			StringUtils::from_chars<uint32_t>(c.index.data(), c.index.data() + c.index.size(), if_v6id);
		OV_ERR_FAIL_COND_V(from_chars.ec != std::errc {}, NetworkError::SOCKET_ERROR);
		if (type == NetworkResolver::Type::IPV6) {
			break; // IPv6 uses index.
		}

		for (IpAddress const& F : c.ip_addresses) {
			if (!F.is_ipv4()) {
				continue; // Wrong IP type.
			}
			if_ip = F;
			break;
		}
		break;
	}

	if (level == IPPROTO_IP) {
		OV_ERR_FAIL_COND_V(!if_ip.is_valid(), NetworkError::INVALID_PARAMETER);
		struct ip_mreq greq; // NOLINT
		int sock_opt = p_add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		std::memcpy(&greq.imr_multiaddr, p_multi_address.get_ipv4().data(), 4);
		std::memcpy(&greq.imr_interface, if_ip.get_ipv4().data(), 4);
		ret = setsockopt(_sock, level, sock_opt, (const char*)&greq, sizeof(greq));
	} else {
		struct ipv6_mreq greq; // NOLINT
		int sock_opt = p_add ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
		std::memcpy(&greq.ipv6mr_multiaddr, p_multi_address.get_ipv6().data(), 16);
		greq.ipv6mr_interface = if_v6id;
		ret = setsockopt(_sock, level, sock_opt, (const char*)&greq, sizeof(greq));
	}

	if (ret == -1) {
		return _get_socket_error();
	}

	return NetworkError::OK;
}

size_t WindowsSocket::_set_addr_storage(
	sockaddr_storage* p_addr, IpAddress const& p_ip, port_type p_port, NetworkResolver::Type p_ip_type
) {
	std::memset(p_addr, 0, sizeof(struct sockaddr_storage));
	if (p_ip_type == NetworkResolver::Type::IPV6 || p_ip_type == NetworkResolver::Type::ANY) { // IPv6 socket.

		// IPv6 only socket with IPv4 address.
		OV_ERR_FAIL_COND_V(!p_ip.is_wildcard() && p_ip_type == NetworkResolver::Type::IPV6 && p_ip.is_ipv4(), 0);

		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)p_addr;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(p_port);
		if (p_ip.is_valid()) {
			std::memcpy(&addr6->sin6_addr.s6_addr, p_ip.get_ipv6().data(), 16);
		} else {
			addr6->sin6_addr = in6addr_any;
		}
		return sizeof(sockaddr_in6);
	} else { // IPv4 socket.

		// IPv4 socket with IPv6 address.
		OV_ERR_FAIL_COND_V(!p_ip.is_wildcard() && !p_ip.is_ipv4(), 0);

		struct sockaddr_in* addr4 = (struct sockaddr_in*)p_addr;
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(p_port); // Short, network byte order.

		if (p_ip.is_valid()) {
			std::memcpy(&addr4->sin_addr.s_addr, p_ip.get_ipv4().data(), 4);
		} else {
			addr4->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof(sockaddr_in);
	}
}

#endif
