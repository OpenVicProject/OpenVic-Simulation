#pragma once

#include <concepts>
#include <functional>

namespace OpenVic {
#define FWD(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)
	template<typename Fn, typename Return = void, typename... Args>
	concept Functor = requires(Fn&& fn, Args&&... args) {
		{ std::invoke(FWD(fn), FWD(args)...) } -> std::same_as<Return>;
	};

	template<typename Fn, typename Return = void, typename... Args>
	concept FunctorConvertible = requires(Fn&& fn, Args&&... args) {
		{ std::invoke(FWD(fn), FWD(args)...) } -> std::convertible_to<Return>;
	};

	template<typename Fn, typename... Args>
	concept Callback = Functor<Fn, bool, Args...>;
#undef FWD
}