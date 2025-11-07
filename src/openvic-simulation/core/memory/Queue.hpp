#pragma once

#include <queue>

#include "openvic-simulation/core/memory/Deque.hpp"

namespace OpenVic::memory {
	template<typename T, typename Container = memory::deque<T>>
	using queue = std::queue<T, Container>;
}
