#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct HostSession {
		HostSession(std::string game_name = "HostSession");

		std::vector<uint8_t> serialize() const;
		static HostSession deseralize(PacketSpan span);

	private:
		std::string PROPERTY_RW(game_name);
	};
}
