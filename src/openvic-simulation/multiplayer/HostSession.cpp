
#include "HostSession.hpp"

#include <string>

#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"
#include "openvic-simulation/types/Signal.hpp"

using namespace OpenVic;

HostSession::HostSession(std::string game_name) : game_name { game_name } {}

std::vector<uint8_t> HostSession::serialize() const {
	PacketBuilder builder;
	builder.put_back(game_name);
	return builder;
}

HostSession HostSession::deseralize(PacketSpan span) {
	HostSession result;
	result.game_name = span.read<std::string>();
	return result;
}

HostSession& HostSession::operator=(HostSession&& other) {
	std::swap(game_name, other.game_name);
	session_changed(*this);
	return *this;
}

void HostSession::set_game_name(std::string new_game_name) {
	game_name = new_game_name;
	session_changed(*this);
}
