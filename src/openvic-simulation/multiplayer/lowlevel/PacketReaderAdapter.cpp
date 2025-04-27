#include "PacketReaderAdapter.hpp"

template struct OpenVic::PacketReaderAdapter<std::vector<uint8_t>>;
template struct OpenVic::PacketReaderAdapter<std::span<uint8_t>>;
