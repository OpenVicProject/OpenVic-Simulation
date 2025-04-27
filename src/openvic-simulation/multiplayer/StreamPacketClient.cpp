
#include "StreamPacketClient.hpp"

#include "openvic-simulation/multiplayer/PacketStream.hpp"
#include "openvic-simulation/multiplayer/TcpPacketStream.hpp"

using namespace OpenVic;

template struct OpenVic::BasicStreamPacketClient<PacketStream>;
template struct OpenVic::BasicStreamPacketClient<TcpPacketStream>;
