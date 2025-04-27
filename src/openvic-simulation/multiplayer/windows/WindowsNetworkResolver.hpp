#pragma once

#if !defined(_WIN32)
#error "WindowsNetworkResolver.hpp should only be included on windows systems"
#endif

#include "openvic-simulation/multiplayer/NetworkResolverBase.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct WindowsNetworkResolver final : NetworkResolverBase {
		static WindowsNetworkResolver& singleton() {
			return _singleton;
		}

		OpenVic::string_map_t<InterfaceInfo> get_local_interfaces() const override;

	private:
		friend NetworkResolverBase::ResolveHandler;

		std::vector<IpAddress> _resolve_hostname(std::string_view p_hostname, Type p_type = Type::ANY) const override;

		static WindowsNetworkResolver _singleton;
	};
}
