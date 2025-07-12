#pragma once

#include <concepts>

#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	template<typename TypeTag, std::integral IndexT = size_t>
	class HasIndex {
	public:
		using index_t = IndexT;
	private:
		const index_t PROPERTY(index);

	protected:
		HasIndex(index_t new_index) : index { new_index } {}
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