#pragma once

#include <memory>

#include "openvic-simulation/utility/Concepts.hpp"

namespace OpenVic {
	template<typename InputIt, typename Sentinel, typename ForwardIt, typename Allocator>
	constexpr ForwardIt uninitialized_copy(InputIt first, Sentinel last, ForwardIt result, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_copy(first, last, result);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; first != last; ++first, (void)++result) {
				traits::construct(alloc, std::addressof(*result), *first);
			}
			return result;
		}
	}

	template<typename InputIt, typename Sentinel, typename ForwardIt, typename Allocator>
	constexpr ForwardIt uninitialized_move(InputIt first, Sentinel last, ForwardIt result, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_move(first, last, result);
		} else {
			return uninitialized_copy(std::make_move_iterator(first), std::make_move_iterator(last), result, alloc);
		}
	}

	template<typename ForwardIt, typename Size, typename T, typename Allocator>
	constexpr ForwardIt uninitialized_fill_n(ForwardIt first, Size n, T const& value, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_fill_n(first, n, value);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; n > 0; --n, ++first) {
				traits::construct(alloc, std::addressof(*first), value);
			}
			return first;
		}
	}

	template<typename ForwardIt, typename Size, typename T, typename Allocator>
	constexpr ForwardIt uninitialized_default_n(ForwardIt first, Size n, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			return std::uninitialized_value_construct_n(first, n);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; n > 0; --n, ++first) {
				traits::construct(alloc, std::addressof(*first));
			}
			return first;
		}
	}

	template<typename ForwardIt, typename Allocator>
	constexpr inline void destroy(ForwardIt first, ForwardIt last, Allocator& alloc) {
		if constexpr (specialization_of<Allocator, std::allocator>) {
			std::destroy(first, last);
		} else {
			using traits = std::allocator_traits<Allocator>;
			for (; first != last; ++first) {
				traits::destroy(alloc, std::addressof(*first));
			}
		}
	}
}
