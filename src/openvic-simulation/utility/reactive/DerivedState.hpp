#pragma once

#include "DependencyTracker.hpp"

#include <function2/function2.hpp>

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/reactive/DependencyTracker.hpp"

namespace OpenVic {
	template <typename T>
	struct DerivedState : public DependencyTracker {
	private:
		T cached_value;
		fu2::function<const T(DependencyTracker&)> calculate;

		void recalculate_if_dirty() {
			if (!is_dirty) {
				return;
			}
			const std::lock_guard<std::mutex> lock_guard { is_dirty_lock };
			if (!is_dirty) {
				return;
			}

			T value = calculate(*this);

			if (has_no_connections()) {
				Logger::warning(
					"DEVELOPER WARNING: OpenVic::DerivedState<",
					OpenVic::utility::type_name<T>(),
					"> has no reactive dependencies. Its value will never change again.",
					"If it should be reactive, ensure 'calculate' accesses its dependencies. ",
					"Alternatively use a plain variable or MutableState<T>."
				);
			}

			cached_value = std::move(value);
			is_dirty = false;
		}

	public:
		template<typename Func>
		explicit DerivedState(Func&& new_calculate)
		requires std::is_default_constructible_v<T>
			&& std::is_convertible_v<Func, fu2::function<const T(DependencyTracker&)>>
			: calculate { std::forward<Func>(new_calculate) },
			cached_value() {}

		template<typename Func, typename InitialValue>
		requires std::is_convertible_v<InitialValue, T>
			&& std::is_convertible_v<Func, fu2::function<const T(DependencyTracker&)>>
		explicit DerivedState(Func&& new_calculate, InitialValue&& empty_initial_value)
			: calculate { std::forward<Func>(new_calculate) },
			cached_value { std::forward<InitialValue>(empty_initial_value) } {}

		DerivedState(DerivedState&&) = delete;
		DerivedState(DerivedState const&) = delete;
		DerivedState& operator=(DerivedState&&) = delete;
		DerivedState& operator=(DerivedState const&) = delete;

		template<typename ConnectTemplateType>
		requires std::invocable<ConnectTemplateType, signal<>&>
		[[nodiscard]] T const& get(ConnectTemplateType&& connect) {
			recalculate_if_dirty();
			connect(changed);
			return cached_value;
		}
		[[nodiscard]] T const& get(DependencyTracker& tracker) {
			recalculate_if_dirty();
			tracker.track(changed);
			return cached_value;
		}
		[[nodiscard]] T const& get_untracked() {
			recalculate_if_dirty();
			return cached_value;
		}
		
		//special case where connection may be discarded as the observer handles it
		template<typename Pmf, OpenVic::_detail::signal::Observer Ptr>
		requires OpenVic::_detail::signal::Callable<Pmf, Ptr>
		connection connect_using_observer(Pmf&& pmf, Ptr&& ptr, OpenVic::_detail::signal::group_id gid = 0) {
			return changed.connect(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
		}

		template<typename... Ts>
		[[nodiscard]] connection connect(Ts&&... args) {
			return changed.connect(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		[[nodiscard]] connection connect_extended(Ts&&... args) {
			return changed.connect_extended(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		[[nodiscard]] scoped_connection connect_scoped(Ts&&... args) {
			return changed.connect_scoped(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		void disconnect(Ts&&... args) {
			return changed.disconnect(std::forward<Ts>(args)...);
		}
	};
}
