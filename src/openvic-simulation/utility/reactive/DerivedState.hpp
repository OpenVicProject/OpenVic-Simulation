#pragma once

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/reactive/DependencyTracker.hpp"
#include "openvic-simulation/utility/reactive/TrackableState.hpp"

namespace OpenVic {
	template <typename T>
	struct DerivedState : public TrackableState<T>, public observer {
	private:
		memory::unique_ptr<std::mutex> recalculate_lock;
		std::function<const T()> calculate;
		bool is_dirty = true;

		void mark_dirty() {
			is_dirty = true;
		}

	public:
		DerivedState()
			requires std::is_default_constructible_v<T>
			: DerivedState([]()->T{ return T{}; }) {}

		explicit DerivedState(std::function<T()> const& new_calculate)
			requires std::is_default_constructible_v<T>
			: calculate { new_calculate },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>() {}

		explicit DerivedState(std::function<T()> const& new_calculate, T const& empty_initial_value)
			: calculate { new_calculate },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>(empty_initial_value) {}

		explicit DerivedState(std::function<T()> const& new_calculate, T&& empty_initial_value)
			: calculate { new_calculate },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>(std::move(empty_initial_value)) {}

		explicit DerivedState(std::function<T()>&& new_calculate)
			requires std::is_default_constructible_v<T>
			: calculate { std::move(new_calculate) },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>() {}

		explicit DerivedState(std::function<T()>&& new_calculate, T const& empty_initial_value)
			: calculate { std::move(new_calculate) },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>(empty_initial_value) {}

		explicit DerivedState(std::function<T()>&& new_calculate, T&& empty_initial_value)
			: calculate { std::move(new_calculate) },
			recalculate_lock { memory::make_unique<std::mutex>() },
			TrackableState<T>(std::move(empty_initial_value)) {}

		DerivedState(DerivedState&&) = default;
		DerivedState(DerivedState const&) = delete;
		DerivedState& operator=(DerivedState&&) = delete;
		DerivedState& operator=(DerivedState const&) = delete;
			
		[[nodiscard]] T const& get() {
			if (is_dirty) {
				const std::lock_guard<std::mutex> lock_guard { *recalculate_lock };
				if (is_dirty) {
					observer::disconnect_all();
					DependencyTracker::start_tracking([this](signal<>& dependency_changed) mutable->void {
						dependency_changed.connect_scoped(&DerivedState<T>::mark_dirty, this);
					});
					T value = calculate();
					DependencyTracker::end_tracking();

					if (observer::has_no_connections()) {
						Logger::warning(
							"DEVELOPER WARNING: OpenVic::DerivedState<",
							OpenVic::utility::type_name<T>(),
							"> has no reactive dependencies. Its value will never change again.",
							"If it should be reactive, ensure 'calculate' accesses its dependencies. ",
							"Alternatively use a plain variable or MutableState<T>."
						);
					}

					TrackableState<T>::set_cached_value(std::move(value));
					is_dirty = false;
				}
			}

			return TrackableState<T>::get_cached_value_tracked();
		}
	};
}