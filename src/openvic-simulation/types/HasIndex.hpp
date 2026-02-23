#pragma once

#include <cstddef>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/template/Concepts.hpp"

namespace OpenVic {
	template<typename TypeTag, derived_from_specialization_of<type_safe::strong_typedef> IndexT>
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

		constexpr bool operator==(HasIndex const& rhs) const {
			return index == rhs.index;
		}
	};
}

namespace std {
	template<OpenVic::has_index T>
	struct hash<T> {
		[[nodiscard]] std::size_t operator()(T const& obj) const noexcept {
			return std::hash<typename T::index_t>{}(obj.index);
		}
	};
}