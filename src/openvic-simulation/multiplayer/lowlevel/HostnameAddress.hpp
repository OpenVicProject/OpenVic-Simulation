#pragma once

#include <string_view>

#include "openvic-simulation/multiplayer/lowlevel/IpAddress.hpp"

namespace OpenVic {
	struct HostnameAddress {
		HostnameAddress();
		HostnameAddress(IpAddress const& address);
		HostnameAddress(std::string_view name_or_address);

		IpAddress const& resolved_address() const;
		void set_resolved_address(IpAddress const& address);

	private:
		IpAddress _resolved_address;
	};
}
