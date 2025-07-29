#pragma once

#include "openvic-simulation/types/Signal.hpp"

namespace OpenVic::DependencyTracker {
	void start_tracking(std::function<void(signal<>&)>&& connect);
	void track(signal<>& changed);
	void end_tracking();
}