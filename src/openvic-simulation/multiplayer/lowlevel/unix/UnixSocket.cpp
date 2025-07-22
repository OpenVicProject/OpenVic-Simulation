#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include "UnixSocket.hpp"

#include <bit>
#include <cerrno>
#include <charconv>
#include <span>
#include <system_error>
#include <utility>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkError.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkSocketBase.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

// BSD calls this flag IPV6_JOIN_GROUP
#if !defined(IPV6_ADD_MEMBERSHIP) && defined(IPV6_JOIN_GROUP)
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#if !defined(IPV6_DROP_MEMBERSHIP) && defined(IPV6_LEAVE_GROUP)
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

using namespace OpenVic;

UnixSocket::UnixSocket() {}

UnixSocket::~UnixSocket() {
	close();
}

NetworkError UnixSocket::_get_socket_error() const {
	switch (errno) {
	case EISCONN:	  return NetworkError::IS_CONNECTED;
	case EINPROGRESS:
	case EALREADY:	  return NetworkError::IN_PROGRESS;
#if EAGAIN != EWOULDBLOCK
	case EAGAIN:
#endif
	case EWOULDBLOCK:	return NetworkError::WOULD_BLOCK;
	case EADDRINUSE:
	case EINVAL:
	case EADDRNOTAVAIL: return NetworkError::ADDRESS_INVALID_OR_UNAVAILABLE;
	case EACCES:		return NetworkError::UNAUTHORIZED;
	case ENOBUFS:		return NetworkError::BUFFER_TOO_SMALL;
	default: //
		Logger::info("Socket error: ", strerror(errno));
		return NetworkError::OTHER;
	}
}

void UnixSocket::_set_ip_port(struct sockaddr_storage* addr, IpAddress* r_ip, port_type* r_port) {
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

bool UnixSocket::_can_use_ip(IpAddress const& ip, const bool for_bind) const {
	if (for_bind && !(ip.is_valid() || ip.is_wildcard())) {
		return false;
	} else if (!for_bind && !ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	NetworkResolver::Type type = ip.is_ipv4() ? NetworkResolver::Type::IPV4 : NetworkResolver::Type::IPV6;
	return !(_ip_type != NetworkResolver::Type::ANY && !ip.is_wildcard() && _ip_type != type);
}

void UnixSocket::setup() {}

void UnixSocket::cleanup() {}

NetworkError UnixSocket::open(Type type, NetworkResolver::Type& ip_type) {
	OV_ERR_FAIL_COND_V(is_open(), NetworkError::ALREADY_OPEN);
	OV_ERR_FAIL_COND_V(
		ip_type > NetworkResolver::Type::ANY || ip_type < NetworkResolver::Type::NONE, NetworkError::INVALID_PARAMETER
	);

#if defined(__OpenBSD__)
	// OpenBSD does not support dual stacking, fallback to IPv4 only.
	if (ip_type == NetworkResolver::Type::ANY) {
		ip_type = NetworkResolver::Type::IPV4;
	}
#endif

	int family = ip_type == NetworkResolver::Type::IPV4 ? AF_INET : AF_INET6;
	int protocol = type == Type::TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int socket_type = type == Type::TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, socket_type, protocol);

	if (_sock == -1 && ip_type == NetworkResolver::Type::ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		ip_type = NetworkResolver::Type::IPV4;
		family = AF_INET;
		_sock = socket(family, socket_type, protocol);
	}

	OV_ERR_FAIL_COND_V(_sock == -1, NetworkError::SOCKET_ERROR);
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

	_is_stream = type == Type::TCP;

	// Disable descriptor sharing with subprocesses.
	_set_close_exec_enabled(true);

#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS).
	int par = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		Logger::info("Unable to turn off SIGPIPE on socket.");
	}
#endif
	return NetworkError::OK;
}

void UnixSocket::close() {
	if (_sock != -1) {
		::close(_sock);
	}

	_sock = -1;
	_ip_type = NetworkResolver::Type::NONE;
	_is_stream = false;
}

NetworkError UnixSocket::bind(IpAddress const& addr, port_type p_port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(addr, true), NetworkError::INVALID_PARAMETER);

	sockaddr_storage addr_store; // NOLINT
	size_t addr_size = _set_addr_storage(&addr_store, addr, p_port, _ip_type);

	if (::bind(_sock, (struct sockaddr*)&addr_store, addr_size) != 0) {
		NetworkError err = _get_socket_error();
		Logger::info("Failed to bind socket. Error: ", strerror(errno));
		close();
		return err;
	}

	return NetworkError::OK;
}

NetworkError UnixSocket::listen(int max_pending) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	if (::listen(_sock, max_pending) != 0) {
		NetworkError err = _get_socket_error();
		Logger::info("Failed to listen from socket. Error: ", strerror(errno));
		close();
		return err;
	}

	return NetworkError::OK;
}

NetworkError UnixSocket::connect_to_host(IpAddress const& host, port_type port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(host, false), NetworkError::INVALID_PARAMETER);

	struct sockaddr_storage addr; // NOLINT
	size_t addr_size = _set_addr_storage(&addr, host, port, _ip_type);

	if (::connect(_sock, (struct sockaddr*)&addr, addr_size) != 0) {
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

NetworkError UnixSocket::poll(PollType type, int timeout) const {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	struct pollfd pfd { .fd = _sock, .events = POLLIN, .revents = 0 };

	switch (type) {
		using enum PollType;
	case IN:	 pfd.events = POLLIN; break;
	case OUT:	 pfd.events = POLLOUT; break;
	case IN_OUT: pfd.events = POLLOUT | POLLIN; break;
	}

	int ret = ::poll(&pfd, 1, timeout);

	if (ret < 0 || pfd.revents & POLLERR) {
		NetworkError err = _get_socket_error();
		Logger::info("Error when polling socket. Error ", strerror(errno));
		return err;
	}

	if (ret == 0) {
		return NetworkError::BUSY;
	}

	return NetworkError::OK;
}

NetworkError UnixSocket::receive(uint8_t* buffer, size_t p_len, int64_t& r_read) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	r_read = ::recv(_sock, buffer, p_len, 0);

	if (r_read < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

NetworkError UnixSocket::receive_from( //
	uint8_t* buffer, size_t p_len, int64_t& r_read, IpAddress& r_ip, port_type& r_port, bool peek
) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	struct sockaddr_storage from; // NOLINT
	socklen_t socket_length = sizeof(from);
	std::memset(&from, 0, socket_length);

	r_read = ::recvfrom(_sock, buffer, p_len, peek ? MSG_PEEK : 0, (struct sockaddr*)&from, &socket_length);

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

NetworkError UnixSocket::send(const uint8_t* buffer, size_t p_len, int64_t& r_sent) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	int flags = 0;
#ifdef MSG_NOSIGNAL
	if (_is_stream) {
		flags = MSG_NOSIGNAL;
	}
#endif
	r_sent = ::send(_sock, buffer, p_len, flags);

	if (r_sent < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

NetworkError UnixSocket::send_to(const uint8_t* buffer, size_t p_len, int64_t& r_sent, IpAddress const& ip, port_type port) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);

	struct sockaddr_storage addr; // NOLINT
	size_t addr_size = _set_addr_storage(&addr, ip, port, _ip_type);
	r_sent = ::sendto(_sock, buffer, p_len, 0, (struct sockaddr*)&addr, addr_size);

	if (r_sent < 0) {
		NetworkError err = _get_socket_error();
		if (err == NetworkError::WOULD_BLOCK) {
			return NetworkError::BUSY;
		}
		return err;
	}

	return NetworkError::OK;
}

UnixSocket* UnixSocket::_accept(IpAddress& r_ip, port_type& r_port) {
	OV_ERR_FAIL_COND_V(!is_open(), nullptr);

	struct sockaddr_storage their_addr; // NOLINT
	socklen_t size = sizeof(their_addr);
	int fd = ::accept(_sock, (struct sockaddr*)&their_addr, &size);
	if (fd == -1) {
		NetworkError err = _get_socket_error();
		Logger::info("Error when accepting socket connection. Error: ", strerror(errno));
		return nullptr;
	}

	_set_ip_port(&their_addr, &r_ip, &r_port);

	UnixSocket* ns = new UnixSocket();
	ns->_sock = fd;
	ns->_ip_type = _ip_type;
	ns->_is_stream = _is_stream;
	// Disable descriptor sharing with subprocesses.
	ns->_set_close_exec_enabled(true);
	ns->set_blocking_enabled(false);
	return ns;
}

bool UnixSocket::is_open() const {
	return _sock != -1;
}

int UnixSocket::available_bytes() const {
	OV_ERR_FAIL_COND_V(!is_open(), -1);

	int len;
	int ret = ioctl(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		Logger::info("Error when checking available bytes on socket. Error: ", strerror(errno));
		return -1;
	}
	return len;
}

NetworkError UnixSocket::get_socket_address(IpAddress* r_ip, port_type* r_port) const {
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

NetworkError UnixSocket::set_broadcasting_enabled(bool enabled) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == NetworkResolver::Type::IPV6) {
		return NetworkError::UNSUPPORTED;
	}

	int par = enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &par, sizeof(int)) != 0) {
		Logger::warning("Unable to change broadcast setting.");
		return NetworkError::BROADCAST_CHANGE_FAILED;
	}
	return NetworkError::OK;
}

void UnixSocket::set_blocking_enabled(bool enabled) {
	OV_ERR_FAIL_COND(!is_open());

	int ret = 0;
	int opts = fcntl(_sock, F_GETFL);
	if (enabled) {
		ret = fcntl(_sock, F_SETFL, opts & ~O_NONBLOCK);
	} else {
		ret = fcntl(_sock, F_SETFL, opts | O_NONBLOCK);
	}

	if (ret != 0) {
		Logger::warning("Unable to change non-block mode.");
	}
}

void UnixSocket::set_ipv6_only_enabled(bool enabled) {
	OV_ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	OV_ERR_FAIL_COND(_ip_type == NetworkResolver::Type::IPV4);

	int par = enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, &par, sizeof(int)) != 0) {
		Logger::warning("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void UnixSocket::set_tcp_no_delay_enabled(bool enabled) {
	OV_ERR_FAIL_COND(!is_open());
	OV_ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, &par, sizeof(int)) < 0) {
		Logger::warning("Unable to set TCP no delay option.");
	}
}

void UnixSocket::set_reuse_address_enabled(bool enabled) {
	OV_ERR_FAIL_COND(!is_open());

	int par = enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &par, sizeof(int)) < 0) {
		Logger::warning("Unable to set socket REUSEADDR option.");
	}
}

NetworkError UnixSocket::change_multicast_group(IpAddress const& multi_address, std::string_view if_name, bool add) {
	OV_ERR_FAIL_COND_V(!is_open(), NetworkError::UNCONFIGURED);
	OV_ERR_FAIL_COND_V(!_can_use_ip(multi_address, false), NetworkError::INVALID_PARAMETER);

	// Need to force level and af_family to IP(v4) when using dual stacking and provided multicast group is IPv4.
	NetworkResolver::Type type =
		_ip_type == NetworkResolver::Type::ANY && multi_address.is_ipv4() ? NetworkResolver::Type::IPV4 : _ip_type;
	// This needs to be the proper level for the multicast group, no matter if the socket is dual stacking.
	int level = type == NetworkResolver::Type::IPV4 ? IPPROTO_IP : IPPROTO_IPV6;
	int ret = -1;

	IpAddress if_ip;
	uint32_t if_v6id = 0;
	OpenVic::string_map_t<NetworkResolver::InterfaceInfo> if_info = NetworkResolver::singleton().get_local_interfaces();
	for (std::pair<memory::string, NetworkResolver::InterfaceInfo> const& pair : if_info) {
		NetworkResolver::InterfaceInfo const& c = pair.second;
		if (c.name != if_name) {
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
		int sock_opt = add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		std::memcpy(&greq.imr_multiaddr, multi_address.get_ipv4().data(), 4);
		std::memcpy(&greq.imr_interface, if_ip.get_ipv4().data(), 4);
		ret = setsockopt(_sock, level, sock_opt, (const char*)&greq, sizeof(greq));
	} else {
		struct ipv6_mreq greq; // NOLINT
		int sock_opt = add ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
		std::memcpy(&greq.ipv6mr_multiaddr, multi_address.get_ipv6().data(), 16);
		greq.ipv6mr_interface = if_v6id;
		ret = setsockopt(_sock, level, sock_opt, (const char*)&greq, sizeof(greq));
	}

	if (ret == -1) {
		return _get_socket_error();
	}

	return NetworkError::OK;
}

size_t UnixSocket::_set_addr_storage( //
	sockaddr_storage* addr, IpAddress const& ip, port_type port, NetworkResolver::Type ip_type
) {
	std::memset(addr, 0, sizeof(struct sockaddr_storage));
	if (ip_type == NetworkResolver::Type::IPV6 || ip_type == NetworkResolver::Type::ANY) { // IPv6 socket.

		// IPv6 only socket with IPv4 address.
		OV_ERR_FAIL_COND_V(!ip.is_wildcard() && ip_type == NetworkResolver::Type::IPV6 && ip.is_ipv4(), 0);

		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)addr;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(port);
		if (ip.is_valid()) {
			std::memcpy(&addr6->sin6_addr.s6_addr, ip.get_ipv6().data(), 16);
		} else {
			addr6->sin6_addr = in6addr_any;
		}
		return sizeof(sockaddr_in6);
	} else { // IPv4 socket.

		// IPv4 socket with IPv6 address.
		OV_ERR_FAIL_COND_V(!ip.is_wildcard() && !ip.is_ipv4(), 0);

		struct sockaddr_in* addr4 = (struct sockaddr_in*)addr;
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(port); // Short, network byte order.

		if (ip.is_valid()) {
			std::memcpy(&addr4->sin_addr.s_addr, ip.get_ipv4().data(), 4);
		} else {
			addr4->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof(sockaddr_in);
	}
}

void UnixSocket::_set_close_exec_enabled(bool enabled) {
	// Enable close on exec to avoid sharing with subprocesses. Off by default on Windows.
	int opts = fcntl(_sock, F_GETFD);
	fcntl(_sock, F_SETFD, opts | FD_CLOEXEC);
}

#endif
