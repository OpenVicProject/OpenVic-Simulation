#pragma once

#include <mutex>

#include <function2/function2.hpp>

#include "openvic-simulation/types/Signal.hpp"

namespace OpenVic {
	struct DependencyTracker {
	private:
		memory::vector<scoped_connection> connections;
		std::mutex connections_lock;

		void mark_dirty() {
			if (is_dirty) {
				return;
			}

			const std::lock_guard<std::mutex> lock_guard { is_dirty_lock };
			if (is_dirty) {
				return;
			}

			disconnect_all();
			is_dirty = true;
			changed();
		}
	protected:
		signal<> changed;
		bool is_dirty = true;
		std::mutex is_dirty_lock;
		
		virtual ~DependencyTracker() {
			disconnect_all();
		}

		constexpr bool has_no_connections() const {
			return connections.empty();
		}

		void disconnect_all() {
			const std::lock_guard<std::mutex> lock_guard { connections_lock };
			connections.clear();
		}
	public:
		void track(signal<>& dependency_changed) {
			const std::lock_guard<std::mutex> lock_guard { connections_lock };
			connections.emplace_back(
				dependency_changed.connect(&DependencyTracker::mark_dirty, this)
			);
		}

		template<typename... Args>
		void track(signal<Args...>& dependency_changed) {
			const std::lock_guard<std::mutex> lock_guard { connections_lock };
			connections.emplace_back(
				dependency_changed.connect([this](Args...) { mark_dirty(); })
			);
		}
	};
}
