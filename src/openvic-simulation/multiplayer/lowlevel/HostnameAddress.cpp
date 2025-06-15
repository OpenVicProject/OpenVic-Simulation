
#include "HostnameAddress.hpp"

#include <charconv>
#include <system_error>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"
#include "openvic-simulation/multiplayer/lowlevel/NetworkResolver.hpp"

using namespace OpenVic;

HostnameAddress::HostnameAddress() = default;

HostnameAddress::HostnameAddress(IpAddress const& address) : _resolved_address(address) {}

HostnameAddress::HostnameAddress(std::string_view name_or_address) : HostnameAddress() {
	std::from_chars_result result =
		_resolved_address.from_chars(name_or_address.data(), name_or_address.data() + name_or_address.size());
	if (result.ec != std::errc {}) {
		_resolved_address = NetworkResolver::singleton().resolve_hostname(name_or_address);
	}
}

IpAddress const& HostnameAddress::resolved_address() const {
	return _resolved_address;
}

void HostnameAddress::set_resolved_address(IpAddress const& address) {
	_resolved_address = address;
}
