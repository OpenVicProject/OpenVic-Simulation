#pragma once

#if !defined(_WIN32)
#error "WindowsNetworkResolver.hpp should only be included on windows systems"
#endif

#include "openvic-simulation/multiplayer/lowlevel/NetworkResolverBase.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct WindowsNetworkResolver final : NetworkResolverBase {
		static constexpr Provider provider_value = Provider::WINDOWS;

		WindowsNetworkResolver();
		~WindowsNetworkResolver();

		static WindowsNetworkResolver& singleton() {
			return _singleton;
		}

		OpenVic::string_map_t<InterfaceInfo> get_local_interfaces() const override;

		Provider provider() const override {
			return provider_value;
		}

	private:
		friend NetworkResolverBase::ResolveHandler;

		memory::vector<IpAddress> _resolve_hostname(std::string_view p_hostname, Type p_type = Type::ANY) const override;

		static WindowsNetworkResolver _singleton;
	};
}
