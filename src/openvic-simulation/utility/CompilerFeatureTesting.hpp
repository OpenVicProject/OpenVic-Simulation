#pragma once

#include <execution>

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/iterator/concepts.hpp>
#include <range/v3/range/concepts.hpp>

namespace OpenVic {
	template<ranges::forward_iterator InputIt, ranges::indirectly_unary_invocable<InputIt> UnaryFunc>
	inline constexpr void try_parallel_for_each(InputIt first, InputIt last, UnaryFunc f) {
#ifndef __cpp_lib_execution
		std::for_each(first, last, f);
#else
#ifdef __linux__
		std::for_each(first, last, f);
#else
		if constexpr (__cpp_lib_execution >= 201603L) {
			std::for_each(std::execution::par, first, last, f);
		} else {
			std::for_each(first, last, f);
		}
#endif
#endif
	}

	struct _for_each_fn {
		template<ranges::forward_iterator Iter, ranges::indirectly_unary_invocable<Iter> Fun>
		constexpr void operator()(Iter first, Iter last, Fun func) const {
			try_parallel_for_each(first, last, func);
		}

		template<ranges::forward_range Range, ranges::indirectly_unary_invocable<ranges::iterator_t<Range>> Fun>
		constexpr void operator()(Range&& r, Fun f) const {
			(*this)(ranges::begin(r), ranges::end(r), std::move(f));
		}
	};

	inline constexpr _for_each_fn parallel_for_each {};
}
