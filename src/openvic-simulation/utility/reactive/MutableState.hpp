#pragma once

#include "openvic-simulation/utility/reactive/TrackableState.hpp"

namespace OpenVic {
	template <typename T>
	struct MutableState : public TrackableState<T> {
	public:
		constexpr MutableState()
			requires std::is_default_constructible_v<T>
			: TrackableState<T>() {}
		constexpr explicit MutableState(T const& new_value) : TrackableState<T>(new_value) {}
		constexpr explicit MutableState(T&& new_value) : TrackableState<T>(std::move(new_value)) {}
		MutableState(MutableState&&) = default;
		MutableState(MutableState const&) = delete;
		MutableState& operator=(MutableState&&) = default;
		MutableState& operator=(MutableState const&) = delete;

		[[nodiscard]] T const& get() {
			return TrackableState<T>::get_cached_value_tracked();
		}
		[[nodiscard]] constexpr T const& get_untracked() const {
			return TrackableState<T>::get_cached_value_untracked();
		}

		void set(T&& new_value) {
			TrackableState<T>::set_cached_value(std::move(new_value));
		}

		void set(T const& new_value) {
			TrackableState<T>::set_cached_value(new_value);
		}

		MutableState<T>& operator+=(auto const& rhs) {
			set(get() + rhs);
			return *this;
		}
		MutableState<T>& operator-=(auto const& rhs) {
			set(get() - rhs);
			return *this;
		}
		MutableState<T>& operator*=(auto const& rhs) {
			set(get() * rhs);
			return *this;
		}
		MutableState<T>& operator/=(auto const& rhs) {
			set(get() / rhs);
			return *this;
		}
	};
}