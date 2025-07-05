#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct HostSession {
		signal_property<HostSession, HostSession const&> session_changed;

		HostSession(std::string game_name = "HostSession");

		HostSession(HostSession const&) = delete;
		HostSession& operator=(HostSession const&) = delete;
		HostSession(HostSession&&) = default;
		HostSession& operator=(HostSession&& other);

		std::vector<uint8_t> serialize() const;
		static HostSession deseralize(PacketSpan span);

		void set_game_name(std::string new_game_name);

	private:
		std::string PROPERTY(game_name);
	};
}
