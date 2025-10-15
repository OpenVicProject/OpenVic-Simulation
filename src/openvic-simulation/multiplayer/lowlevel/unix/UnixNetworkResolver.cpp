#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))

#include "UnixNetworkResolver.hpp"

#include <bit>
#include <charconv>
#include <limits>

#include <netdb.h>

#include <fmt/format.h>

#include <range/v3/algorithm/find.hpp>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <ifaddrs.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#include <net/if.h> // Order is important on OpenBSD, leave as last.

using namespace OpenVic;

static IpAddress _sockaddr2ip(struct sockaddr* p_addr) {
	IpAddress ip;

	if (p_addr->sa_family == AF_INET) {
		struct sockaddr_in* addr = (struct sockaddr_in*)p_addr;
		ip.set_ipv4(std::bit_cast<std::span<uint8_t, 4>>(&addr->sin_addr));
	} else if (p_addr->sa_family == AF_INET6) {
		struct sockaddr_in6* addr6 = (struct sockaddr_in6*)p_addr;
		ip.set_ipv6(addr6->sin6_addr.s6_addr);
	}

	return ip;
}

UnixNetworkResolver UnixNetworkResolver::_singleton {};

string_map_t<UnixNetworkResolver::InterfaceInfo> UnixNetworkResolver::get_local_interfaces() const {
	string_map_t<UnixNetworkResolver::InterfaceInfo> result;

	struct ifaddrs* ifAddrStruct = nullptr;
	struct ifaddrs* ifa = nullptr;
	int family;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family != AF_INET && family != AF_INET6) {
			continue;
		}

		string_map_t<UnixNetworkResolver::InterfaceInfo>::iterator it = result.find(ifa->ifa_name);
		if (it == result.end()) {
			InterfaceInfo info;
			info.name = ifa->ifa_name;
			info.name_friendly = ifa->ifa_name;

			struct stack_string : StackString<fmt::detail::count_digits(std::numeric_limits<uint32_t>::max())> {
				using StackString::_array;
				using StackString::_string_size;
				using StackString::StackString;
			} str {};
			std::to_chars_result to_chars =
				StringUtils::to_chars(str._array.data(), str._array.data() + str.array_length, if_nametoindex(ifa->ifa_name));
			str._string_size = to_chars.ptr - str.data();

			info.index = str;
			auto pair = result.insert_or_assign(ifa->ifa_name, info);
			OV_ERR_CONTINUE(!pair.second);
			it = pair.first;
		}

		InterfaceInfo& info = it.value();
		info.ip_addresses.push_back(_sockaddr2ip(ifa->ifa_addr));
	}

	if (ifAddrStruct != nullptr) {
		freeifaddrs(ifAddrStruct);
	}

	return result;
}

memory::vector<IpAddress> UnixNetworkResolver::_resolve_hostname(std::string_view p_hostname, Type p_type) const {
	struct addrinfo hints; // NOLINT
	struct addrinfo* result = nullptr;

	std::memset(&hints, 0, sizeof(struct addrinfo));
	if (p_type == Type::IPV4) {
		hints.ai_family = AF_INET;
	} else if (p_type == Type::IPV6) {
		hints.ai_family = AF_INET6;
		hints.ai_flags = 0;
	} else {
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_ADDRCONFIG;
	}
	hints.ai_flags &= ~AI_NUMERICHOST;

	int s = getaddrinfo(p_hostname.data(), nullptr, &hints, &result);
	if (s != 0) {
		Logger::info("getaddrinfo failed! Cannot resolve hostname.");
		return {};
	}

	if (result == nullptr || result->ai_addr == nullptr) {
		Logger::info("Invalid response from getaddrinfo.");
		if (result) {
			freeaddrinfo(result);
		}
		return {};
	}

	struct addrinfo* next = result;

	memory::vector<IpAddress> result_addrs;
	do {
		if (next->ai_addr == nullptr) {
			next = next->ai_next;
			continue;
		}
		IpAddress ip = _sockaddr2ip(next->ai_addr);
		if (ip.is_valid() && ranges::find(result_addrs, ip) == result_addrs.end()) {
			result_addrs.push_back(ip);
		}
		next = next->ai_next;
	} while (next);

	freeaddrinfo(result);

	return result_addrs;
}

#endif
