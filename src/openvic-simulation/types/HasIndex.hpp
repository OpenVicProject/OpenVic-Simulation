#pragma once

#include <cstddef>
#include <type_traits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/template/Functional.hpp" // IWYU pragma: keep: bug fix for std::reference_wrapper hash & equality

namespace OpenVic {
	template<typename SelfT, derived_from_specialization_of<type_safe::strong_typedef> IndexT>
	class HasIndex {
	public:
		using index_t = IndexT;

	protected:
		constexpr HasIndex(index_t new_index) : index { new_index } {}
		HasIndex(HasIndex const&) = default;

	public:
		// always 0-based, this may be used for indexing arrays
		const index_t index;

		HasIndex(HasIndex&&) = default;
		HasIndex& operator=(HasIndex const&) = delete;
		HasIndex& operator=(HasIndex&&) = delete;

		friend constexpr bool operator==(HasIndex const& lhs, HasIndex const& rhs) {
			return lhs.index == rhs.index;
		}
	};
}

namespace std {
	template<typename T>
	requires OpenVic::derived_from_specialization_of<std::decay_t<T>, OpenVic::HasIndex>
	struct hash<T> {
		[[nodiscard]] std::size_t operator()(T const& obj) const noexcept {
			return std::hash<typename std::decay_t<T>::index_t> {}(obj.index);
		}
	};
}
