#pragma once


#include <stack>

#include "openvic-simulation/core/memory/Deque.hpp"

namespace OpenVic::memory {
	template<typename T, typename Container = memory::deque<T>>
	using stack = std::stack<T, Container>;
}
