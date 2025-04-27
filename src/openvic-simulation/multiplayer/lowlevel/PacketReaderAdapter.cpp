#include "PacketReaderAdapter.hpp"

#include "openvic-simulation/utility/Containers.hpp"

template struct OpenVic::PacketReaderAdapter<OpenVic::memory::vector<uint8_t>>;
template struct OpenVic::PacketReaderAdapter<std::span<uint8_t>>;
