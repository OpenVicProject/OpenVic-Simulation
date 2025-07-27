#pragma once

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/reactive/DependencyTracker.hpp"

namespace OpenVic {
	template <typename T>
	struct TrackableState {
	private:
		T cached_value;
		signal<> changed;

	protected:
		constexpr TrackableState()
			requires std::is_default_constructible_v<T> {}
		constexpr explicit TrackableState(T const& new_cached_value) : cached_value { new_cached_value } {}
		constexpr explicit TrackableState(T&& new_cached_value) : cached_value { std::move(new_cached_value) } {}
		TrackableState(TrackableState&&) = default;
		TrackableState(TrackableState const&) = delete;
		TrackableState& operator=(TrackableState&&) = default;
		TrackableState& operator=(TrackableState const&) = delete;

		[[nodiscard]] T const& get_cached_value_tracked() {
			DependencyTracker::track(changed);
			return cached_value;
		}

		[[nodiscard]] constexpr T const& get_cached_value_untracked() const {
			return cached_value;
		}

		void set_cached_value(T&& new_cached_value) {
			if (cached_value != new_cached_value) {
				cached_value = std::move(new_cached_value);
				changed();
			}
		}

		void set_cached_value(T const& new_cached_value) {
			if (cached_value != new_cached_value) {
				cached_value = new_cached_value;
				changed();
			}
		}

	public:
		//special case where connection may be discarded as the observer handles it
		template<typename Pmf, OpenVic::_detail::signal::Observer Ptr>
		requires OpenVic::_detail::signal::Callable<Pmf, Ptr>
		connection connect_using_observer(Pmf&& pmf, Ptr&& ptr, OpenVic::_detail::signal::group_id gid = 0) {
			return changed.connect(pmf, ptr, gid);
		}

		template<typename... Ts>
		[[nodiscard]] connection connect(Ts&&... args) const {
			return changed.connect(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		[[nodiscard]] connection connect_extended(Ts&&... args) const {
			return changed.connect_extended(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		[[nodiscard]] scoped_connection connect_scoped(Ts&&... args) const {
			return changed.connect_scoped(std::forward<Ts>(args)...);
		}

		template<typename... Ts>
		void disconnect(Ts&&... args) const {
			return changed.disconnect(std::forward<Ts>(args)...);
		}
	};
#define STATE_PROPERTY(NAME) STATE_PROPERTY_ACCESS(NAME, private)
#define STATE_PROPERTY_ACCESS(NAME, ACCESS) \
	NAME; \
\
public: \
	[[nodiscard]] constexpr auto get_##NAME() const -> decltype(OpenVic::_get_property<decltype(NAME)>(NAME)) { \
		return OpenVic::_get_property<decltype(NAME)>(NAME); \
	} \
	[[nodiscard]] auto get_##NAME##_value() { \
		return NAME.get(); \
	} \
	ACCESS:
}