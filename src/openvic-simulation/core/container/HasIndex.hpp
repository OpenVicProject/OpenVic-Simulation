#pragma once

#include <concepts>
#include <cstddef>

#include "openvic-simulation/core/Property.hpp"

namespace OpenVic {
	template<typename TypeTag, std::integral IndexT = std::size_t>
	class HasIndex {
	public:
		using index_t = IndexT;

	private:
		// always 0-based, this may be used for indexing arrays
		const index_t PROPERTY(index);

	protected:
		constexpr HasIndex(index_t new_index) : index { new_index } {}
		HasIndex(HasIndex const&) = default;

	public:
		HasIndex(HasIndex&&) = default;
		HasIndex& operator=(HasIndex const&) = delete;
		HasIndex& operator=(HasIndex&&) = delete;

		constexpr bool operator==(HasIndex const& rhs) const {
			return index == rhs.index;
		}
	};
}
