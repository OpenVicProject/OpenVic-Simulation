#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include "openvic-simulation/types/AnyRef.hpp"

namespace OpenVic {
	// Based on https://github.com/think-cell/think-cell-library/blob/b9c84dd7fc926fad80829ed49705fa51afe36e87/tc/base/ref.h
	//     Boost Software License - Version 1.0 - August 17th, 2003

	// Permission is hereby granted, free of charge, to any person or organization
	// obtaining a copy of the software and accompanying documentation covered by
	// this license (the "Software") to use, reproduce, display, distribute,
	// execute, and transmit the Software, and to prepare derivative works of the
	// Software, and to permit third-parties to whom the Software is furnished to
	// do so, all subject to the following:

	// The copyright notices in the Software and this entire statement, including
	// the above license grant, this restriction and the following disclaimer,
	// must be included in all copies of the Software, in whole or in part, and
	// all derivative works of the Software, unless such copies or derivative
	// works are solely in the form of machine-executable object code generated by
	// a source language processor.

	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
	// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
	// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
	// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	// DEALINGS IN THE SOFTWARE.
	template<bool bNoExcept, typename Ret, typename... Args>
	struct FunctionRefBase {
		template<bool bNoExcept2, typename Ret2, typename... Args2>
		using type_erased_function_ptr = Ret2 (*)(AnyRef, Args2...) noexcept(bNoExcept2);

		template<bool bNoExcept2, typename Func, typename Ret2, typename... Args2>
		struct make_type_erased_function_ptr final {
			type_erased_function_ptr<bNoExcept2, Ret2, Args2...> operator()() const& noexcept {
				return [](AnyRef anyref,
						  Args2... args) noexcept(bNoExcept2) -> Ret2 { // implicit cast of stateless lambda to function pointer
					return std::invoke(
						anyref.template get_ref<Func>(), std::forward<Args2>(args)...
					); // MAYTHROW unless bNoExcept
				};
			}
		};

		template<bool bNoExcept2, typename Func, typename... Args2>
		struct make_type_erased_function_ptr<bNoExcept2, Func, void, Args2...> final {
			type_erased_function_ptr<bNoExcept2, void, Args2...> operator()() const& noexcept {
				return [](AnyRef anyref,
						  Args2... args) noexcept(bNoExcept2) { // implicit cast of stateless lambda to function pointer
					std::invoke(anyref.template get_ref<Func>(), std::forward<Args2>(args)...); // MAYTHROW unless bNoExcept
				};
			}
		};

		template<typename T>
		[[nodiscard]] static constexpr T& as_lvalue(T&& t) noexcept {
			return t;
		}

		FunctionRefBase(FunctionRefBase const&) noexcept = default;

		Ret operator()(Args... args) const& noexcept(bNoExcept) {
			return m_pfuncTypeErased(m_anyref, std::forward<Args>(args)...); // MAYTHROW unless bNoExcept
		}


		template<typename Derived, typename Base>
		struct decayed_derived_from : std::bool_constant<std::derived_from<typename std::decay<Derived>::type, Base>> {
			static_assert(std::same_as<std::decay_t<Base>, Base>);
		};

		template<typename Func>
		requires(!decayed_derived_from<Func, FunctionRefBase>::value) &&
					std::invocable<std::remove_reference_t<Func>&, Args...> &&
					(std::convertible_to<
						 decltype(std::invoke(std::declval<std::remove_reference_t<Func>&>(), std::declval<Args>()...)), Ret> ||
					 std::is_void<Ret>::value)
		FunctionRefBase(Func&& func) noexcept
			: m_pfuncTypeErased(make_type_erased_function_ptr<bNoExcept, std::remove_reference_t<Func>, Ret, Args...> {}()),
			  m_anyref(as_lvalue(func)) {
			static_assert(
				!std::is_member_function_pointer<Func>::value,
				"Raw member functions are not supported (how would you call them?).  Pass in a lambda or use "
				"std::mem_fn instead."
			);
			static_assert(!std::is_pointer<Func>::value, "Pass in functions rather than function pointers.");
			// Checking the noexcept value of the function call is commented out because MAYTHROW is widely used in generic code
			// such as for_each, range_adaptor... static_assert(!bNoExcept ||
			// noexcept(std::declval<tc::decay_t<Func>&>()(std::declval<Args>()...)));
		}

	private:
		type_erased_function_ptr<bNoExcept, Ret, Args...> m_pfuncTypeErased;
		AnyRef m_anyref;
	};

	template<typename Sig>
	struct FunctionRef;

	template<typename Ret, typename... Args>
	struct FunctionRef<Ret(Args...)> : FunctionRefBase</*bNoExcept*/ false, Ret, Args...> {
		using base_ = typename FunctionRef::FunctionRefBase;
		using base_::base_;
	};

	template<typename Ret, typename... Args>
	struct FunctionRef<Ret(Args...) noexcept> : FunctionRefBase</*bNoExcept*/ true, Ret, Args...> {
		using base_ = typename FunctionRef::FunctionRefBase;
		using base_::base_;
	};
}