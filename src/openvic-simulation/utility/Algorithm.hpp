#pragma once

#include <utility>

namespace OpenVic {
	template<typename InputIt, typename Pred>
	[[nodiscard]] inline constexpr InputIt find_if_dual_adjacent(InputIt begin, InputIt end, Pred&& predicate) {
		if (end - begin <= 2) {
			return end;
		}
		++begin;
		for (; begin != end - 1; ++begin) {
			if (predicate(*(begin - 1), *begin, *(begin + 1))) {
				return begin;
			}
		}
		return end;
	}

	template<typename ForwardIt, typename Pred>
	[[nodiscard]] inline constexpr ForwardIt remove_if_dual_adjacent(ForwardIt begin, ForwardIt end, Pred&& predicate) {
		begin = find_if_dual_adjacent(begin, end, std::forward<Pred>(predicate));
		if (begin != end) {
			for (ForwardIt it = begin; ++it != end - 1;) {
				if (!predicate(*(it - 1), *it, *(it + 1))) {
					*begin++ = std::move(*it);
				}
			}
			*begin++ = std::move(*(end - 1));
			return begin;
		}
		return end;
	}
}
