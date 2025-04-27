#pragma once

#if !(defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#error "UnixNetworkResolver.hpp should only be included on unix systems"
#endif

#include "openvic-simulation/multiplayer/lowlevel/NetworkResolverBase.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct UnixNetworkResolver final : NetworkResolverBase {
		static constexpr Provider provider_value = Provider::UNIX;

		static UnixNetworkResolver& singleton() {
			return _singleton;
		}

		OpenVic::string_map_t<InterfaceInfo> get_local_interfaces() const override;

		Provider provider() const override {
			return provider_value;
		}

	private:
		friend NetworkResolverBase::ResolveHandler;

		memory::vector<IpAddress> _resolve_hostname(std::string_view p_hostname, Type p_type = Type::ANY) const override;

		static UnixNetworkResolver _singleton;
	};
}
