#pragma once

#include <concepts>

namespace OpenVic {
	template<typename TypeTag, std::integral IndexT = size_t>
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