#pragma once

#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/utility/Concepts.hpp"
#include "openvic-simulation/utility/reactive/DependencyTracker.hpp"

namespace OpenVic {
	template <typename T>
	struct ReadOnlyMutableState {
	private:
		signal<T> changed;
	protected:
		T value;

		constexpr ReadOnlyMutableState() requires std::is_default_constructible_v<T> : value() {}
		constexpr explicit ReadOnlyMutableState(T const& new_value) : value { new_value } {}
		constexpr explicit ReadOnlyMutableState(T&& new_value) : value { std::move(new_value) } {}
		ReadOnlyMutableState(ReadOnlyMutableState&&) = delete;
		ReadOnlyMutableState(ReadOnlyMutableState const&) = delete;
		ReadOnlyMutableState& operator=(ReadOnlyMutableState&&) = delete;
		ReadOnlyMutableState& operator=(ReadOnlyMutableState const&) = delete;

		void _set(T&& new_value) {
			value = std::move(new_value);
			changed(value);
		}

		void _set(T const& new_value) {
			value = new_value;
			changed(value);
		}
	public:
		template<typename ConnectTemplateType>
		requires std::invocable<ConnectTemplateType, signal<T>&>
		[[nodiscard]] T const& get(ConnectTemplateType&& connect) {
			connect(changed);
			return value;
		}
		[[nodiscard]] T const& get(DependencyTracker& tracker) {
			tracker.track(changed);
			return value;
		}
		[[nodiscard]] constexpr T const& get_untracked() const {
			return value;
		}
		
		//special case where connection may be discarded as the observer handles it
		template<typename Pmf, OpenVic::_detail::signal::Observer Ptr>
		requires OpenVic::_detail::signal::Callable<Pmf, Ptr, T>
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
	template <typename T>
	struct MutableState : public ReadOnlyMutableState<T> {
		constexpr MutableState() requires std::is_default_constructible_v<T> : ReadOnlyMutableState<T>() {}
		constexpr explicit MutableState(T const& new_value) : ReadOnlyMutableState<T>(new_value) {}
		constexpr explicit MutableState(T&& new_value) : ReadOnlyMutableState<T>(std::move(new_value)) {}
		MutableState(MutableState&&) = delete;
		MutableState(MutableState const&) = delete;
		MutableState& operator=(MutableState&&) = delete;
		MutableState& operator=(MutableState const&) = delete;

		using ReadOnlyMutableState<T>::_set;
		void set(T&& new_value) {
			_set(std::move(new_value));
		}

		void set(T const& new_value) {
			_set(new_value);
		}

		using ReadOnlyMutableState<T>::value;
		MutableState<T>& operator++() requires pre_incrementable <T> {
			set(++value);
			return *this;
		}

		void operator++(int) requires post_incrementable <T> {
			set(value++);
		}

		MutableState<T>& operator--() requires pre_decrementable<T> {
			set(--value);
			return *this;
		}

		void operator--(int) requires post_decrementable<T> {
			set(value--);
		}

		template<typename RHS>
		requires addable<T, RHS, T>
		MutableState<T>& operator+=(RHS const& rhs) {
			set(value + rhs);
			return *this;
		}
		template<typename RHS>
		requires subtractable<T, RHS, T>
		MutableState<T>& operator-=(RHS const& rhs) {
			set(value - rhs);
			return *this;
		}
		template<typename RHS>
		requires multipliable<T, RHS, T>
		MutableState<T>& operator*=(RHS const& rhs) {
			set(value * rhs);
			return *this;
		}
		template<typename RHS>
		requires divisible<T, RHS, T>
		MutableState<T>& operator/=(RHS const& rhs) {
			set(value / rhs);
			return *this;
		}
	};
}

#define STATE_PROPERTY(T, NAME, ...) STATE_PROPERTY_ACCESS(T, NAME, private, __VA_ARGS__)
#define STATE_PROPERTY_ACCESS(T, NAME, ACCESS, ...) \
	MutableState<T> NAME __VA_OPT__({) __VA_ARGS__ __VA_OPT__(}); \
\
public: \
	[[nodiscard]] constexpr ReadOnlyMutableState<T>& get_##NAME() { \
		return NAME; \
	} \
	template<typename ConnectTemplateType> \
	requires std::invocable<ConnectTemplateType, signal<T>&> \
	[[nodiscard]] T const& get_##NAME(ConnectTemplateType&& connect) { \
		return NAME.get(std::forward<ConnectTemplateType>(connect)); \
	} \
	[[nodiscard]] T get_##NAME(DependencyTracker& tracker) { \
		return NAME.get(tracker); \
	} \
	[[nodiscard]] T get_##NAME##_untracked() const { \
		return NAME.get_untracked(); \
	} \
	ACCESS:
