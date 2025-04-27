#pragma once

#ifdef _WIN32
#include "openvic-simulation/multiplayer/lowlevel/windows/WindowsNetworkResolver.hpp"
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include "openvic-simulation/multiplayer/lowlevel/unix/UnixNetworkResolver.hpp"
#else
#error "NetworkResolver.hpp only supports unix or windows systems"
#endif

namespace OpenVic {
	using NetworkResolver =
#ifdef _WIN32
		WindowsNetworkResolver
#else
		UnixNetworkResolver
#endif
		;
}
