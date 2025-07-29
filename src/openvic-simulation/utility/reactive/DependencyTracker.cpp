#include "DependencyTracker.hpp"

#include <mutex>

namespace OpenVic::DependencyTracker {
	static thread_local std::mutex _mutex;
	static thread_local memory::vector<std::function<void(signal<>&)>> _connect_methods;

	void start_tracking(std::function<void(signal<>&)>&& connect) {
		const std::lock_guard<std::mutex> lock_guard { _mutex };
		_connect_methods.push_back(std::move(connect));
	}

	void track(signal<>& changed) {
		const std::lock_guard<std::mutex> lock_guard { _mutex };
		if (_connect_methods.empty()) {
			return;
		}
		_connect_methods.back()(changed);
	}

	void end_tracking() {
		const std::lock_guard<std::mutex> lock_guard { _mutex };
		_connect_methods.pop_back();
	}
}
