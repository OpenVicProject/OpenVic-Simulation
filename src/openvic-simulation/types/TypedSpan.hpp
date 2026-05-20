#pragma once

#include <span>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"

namespace OpenVic {
	template<
		is_strongly_typed IndexType,
		typename ValueType,
		size_t _Extent = std::dynamic_extent
	>
	struct TypedSpan : public forwardable_span<ValueType, _Extent> {
	public:
		using forwardable_span<ValueType, _Extent>::forwardable_span;

		template<typename OtherT>
		constexpr TypedSpan(OtherT& other)
			: forwardable_span<ValueType, _Extent>(
				other.data(),
				get_index_as_size_t(other.size())
			) { }

		[[nodiscard]] constexpr IndexType size() const {
			return IndexType(forwardable_span<ValueType, _Extent>::size());
		}

		[[nodiscard]] constexpr forwardable_span<ValueType, _Extent>::reference operator[](const IndexType index) const {
			assert(index < size());
			return forwardable_span<ValueType, _Extent>::operator[](static_cast<std::size_t>(type_safe::get(index)));
		}

		[[nodiscard]] constexpr operator TypedSpan<IndexType, const ValueType, _Extent>() const {
			return TypedSpan<IndexType, const ValueType, _Extent>{*this};
		}
	};
}
