
#include "StreamPacketClient.hpp"

#include "openvic-simulation/multiplayer/lowlevel/PacketStream.hpp"
#include "openvic-simulation/multiplayer/lowlevel/TcpPacketStream.hpp"

using namespace OpenVic;

template struct OpenVic::BasicStreamPacketClient<PacketStream>;
template struct OpenVic::BasicStreamPacketClient<TcpPacketStream>;
