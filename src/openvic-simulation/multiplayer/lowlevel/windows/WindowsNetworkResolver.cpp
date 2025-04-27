#ifdef _WIN32
#include "WindowsNetworkResolver.hpp"

#include <charconv>
#include <cstdint>

#include <stdio.h>
#include <string.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <range/v3/algorithm/find.hpp>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
//
#include <iphlpapi.h>
#include <ws2tcpip.h>
#undef WIN32_LEAN_AND_MEAN
// Thank You Microsoft
#undef SOCKET_ERROR
#undef IN
#undef OUT

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/windows/WindowsSocket.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

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

WindowsNetworkResolver WindowsNetworkResolver::_singleton {};

WindowsNetworkResolver::WindowsNetworkResolver() {
	WindowsSocket::setup();
}

WindowsNetworkResolver::~WindowsNetworkResolver() {
	WindowsSocket::cleanup();
}

string_map_t<WindowsNetworkResolver::InterfaceInfo> WindowsNetworkResolver::get_local_interfaces() const {
	ULONG buf_size = 1024;
	IP_ADAPTER_ADDRESSES* addrs;

	while (true) {
		addrs = (IP_ADAPTER_ADDRESSES*)malloc(buf_size);
		int err = GetAdaptersAddresses(
			AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
			nullptr, addrs, &buf_size
		);
		if (err == NO_ERROR) {
			break;
		}
		free(addrs);
		if (err == ERROR_BUFFER_OVERFLOW) {
			continue; // Will go back and alloc the right size.
		}

		OV_ERR_FAIL_V_MSG(
			string_map_t<InterfaceInfo> {}, fmt::format("Call to GetAdaptersAddresses failed with error {}.", err)
		);
	}

	string_map_t<InterfaceInfo> result;
	IP_ADAPTER_ADDRESSES* adapter = addrs;

	while (adapter != nullptr) {
		InterfaceInfo info;
		info.name = adapter->AdapterName;

		size_t name_wlength = wcslen(adapter->FriendlyName);
		int name_length = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, name_wlength, 0, 0, nullptr, nullptr);
		info.name_friendly.reserve(name_length * sizeof(char));
		WideCharToMultiByte(
			CP_UTF8, 0, adapter->FriendlyName, name_wlength, info.name_friendly.data(), name_length, nullptr, nullptr
		);

		struct stack_string : StackString<fmt::detail::count_digits(std::numeric_limits<uint32_t>::max())> {
			using StackString::_array;
			using StackString::_string_size;
			using StackString::StackString;
		} str {};
		std::to_chars_result to_chars =
			StringUtils::to_chars(str._array.data(), str._array.data() + str.array_length, adapter->IfIndex);
		str._string_size = to_chars.ptr - str.data();

		info.index = str;

		IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress;
		while (address != nullptr) {
			int family = address->Address.lpSockaddr->sa_family;
			if (family != AF_INET && family != AF_INET6) {
				continue;
			}
			info.ip_addresses.push_back(_sockaddr2ip(address->Address.lpSockaddr));
			address = address->Next;
		}
		adapter = adapter->Next;
		// Only add interface if it has at least one IP.
		if (info.ip_addresses.size() > 0) {
			auto pair = result.insert_or_assign(info.name, info);
			OV_ERR_CONTINUE(!pair.second);
		}
	}

	free(addrs);

	return result;
}

memory::vector<IpAddress> WindowsNetworkResolver::_resolve_hostname(std::string_view p_hostname, Type p_type) const {
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
