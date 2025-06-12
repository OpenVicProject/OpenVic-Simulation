
#include "HostSession.hpp"

#include <string>

#include "openvic-simulation/multiplayer/lowlevel/PacketBuilder.hpp"
#include "openvic-simulation/multiplayer/lowlevel/PacketReaderAdapter.hpp"

using namespace OpenVic;

HostSession::HostSession(std::string game_name) : game_name { game_name } {}

std::vector<uint8_t> HostSession::serialize() const {
	PacketBuilder builder;
	builder.put_back(game_name);
	return builder;
}

HostSession HostSession::deseralize(PacketSpan span) {
	HostSession result { span.read<std::string>() };
	return result;
}
