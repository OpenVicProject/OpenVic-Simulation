#pragma once

#ifdef _WIN32
#include "openvic-simulation/multiplayer/lowlevel/windows/WindowsSocket.hpp"
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include "openvic-simulation/multiplayer/lowlevel/unix/UnixSocket.hpp"
#else
#error "NetworkSocket.hpp only supports unix or windows systems"
#endif

namespace OpenVic {
	using NetworkSocket =
#ifdef _WIN32
		WindowsSocket
#else
		UnixSocket
#endif
		;
}
