#pragma once

#include <span>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/utility/Concepts.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	template<
		derived_from_specialization_of<type_safe::strong_typedef> IndexType,
		typename ValueType,
		size_t _Extent = std::dynamic_extent
	>
	struct TypedSpan : public utility::forwardable_span<ValueType, _Extent> {
	public:
		using utility::forwardable_span<ValueType, _Extent>::forwardable_span;

		constexpr IndexType size() const {
			return IndexType(utility::forwardable_span<ValueType, _Extent>::size());
		}

		constexpr utility::forwardable_span<ValueType, _Extent>::reference operator[](const IndexType _Off) const {
			return utility::forwardable_span<ValueType, _Extent>::operator[](static_cast<std::size_t>(type_safe::get(_Off)));
		}

		constexpr operator TypedSpan<IndexType, const ValueType, _Extent>() {
			return TypedSpan<IndexType, const ValueType, _Extent>{*this};
		}
	};
}