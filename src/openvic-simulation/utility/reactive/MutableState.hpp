#pragma once

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/MathConcepts.hpp"
#include "openvic-simulation/utility/reactive/DependencyTracker.hpp"

namespace OpenVic {
	template <typename T>
	struct MutableState {
	private:
		T value;
		signal<T> changed;
	public:
		constexpr MutableState() requires std::is_default_constructible_v<T> : value() {}
		constexpr explicit MutableState(T const& new_value) : value { new_value } {}
		constexpr explicit MutableState(T&& new_value) : value { std::move(new_value) } {}
		MutableState(MutableState&&) = delete;
		MutableState(MutableState const&) = delete;
		MutableState& operator=(MutableState&&) = delete;
		MutableState& operator=(MutableState const&) = delete;

		[[nodiscard]] T const& get(DependencyTracker& tracker) {
			tracker.track(changed);
			return value;
		}
		[[nodiscard]] constexpr T const& get_untracked() const {
			return value;
		}

		void set(T&& new_value) {
			value = std::move(new_value);
			changed(value);
		}

		void set(T const& new_value) {
			value = new_value;
			changed(value);
		}
		
		MutableState<T>& operator++() requires PreIncrementable<T> {
			set(++value);
			return *this;
		}

		void operator++(int) requires PostIncrementable<T> {
			set(value++);
		}

		MutableState<T>& operator--() requires PreDecrementable<T> {
			set(--value);
			return *this;
		}

		void operator--(int) requires PostDecrementable<T> {
			set(value--);
		}

		template<typename RHS>
		requires Addable<T, RHS, T>
		MutableState<T>& operator+=(RHS const& rhs) {
			set(value + rhs);
			return *this;
		}
		template<typename RHS>
		requires Subtractable<T, RHS, T>
		MutableState<T>& operator-=(RHS const& rhs) {
			set(value - rhs);
			return *this;
		}
		template<typename RHS>
		requires Multipliable<T, RHS, T>
		MutableState<T>& operator*=(RHS const& rhs) {
			set(value * rhs);
			return *this;
		}
		template<typename RHS>
		requires Divisible<T, RHS, T>
		MutableState<T>& operator/=(RHS const& rhs) {
			set(value / rhs);
			return *this;
		}
		
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

#define STATE_PROPERTY(T, NAME, ...) STATE_PROPERTY_ACCESS(T, NAME, private, __VA_ARGS__)
#define STATE_PROPERTY_ACCESS(T, NAME, ACCESS, ...) \
	MutableState<T> NAME __VA_OPT__({) __VA_ARGS__ __VA_OPT__(}); \
\
public: \
	[[nodiscard]] T get_##NAME(DependencyTracker& tracker) { \
		return NAME.get(tracker); \
	} \
	[[nodiscard]] T get_##NAME##_untracked() const { \
		return NAME.get_untracked(); \
	} \
	ACCESS:
}
