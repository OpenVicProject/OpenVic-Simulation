#pragma once

#include <ostream>

#include "openvic-simulation/types/RingBuffer.hpp"

namespace OpenVic {

	template<typename T, typename Allocator = std::allocator<T>>
	struct ValueHistory : private RingBuffer<T, Allocator> {
		using base_type = RingBuffer<T, Allocator>;

		using typename base_type::allocator_type;
		using typename base_type::size_type;
		using typename base_type::iterator;
		using typename base_type::const_iterator;
		using typename base_type::reverse_iterator;
		using typename base_type::const_reverse_iterator;
		using typename base_type::reference;
		using typename base_type::const_reference;
		using typename base_type::pointer;
		using typename base_type::const_pointer;
		using typename base_type::difference_type;
		using base_type::operator[];
		using base_type::size;
		using base_type::empty;
		using base_type::begin;
		using base_type::end;
		using base_type::cbegin;
		using base_type::cend;
		using base_type::rbegin;
		using base_type::rend;
		using base_type::crbegin;
		using base_type::crend;
		using base_type::front;
		using base_type::back;
		using base_type::push_back;

		constexpr ValueHistory() {};
		explicit ValueHistory(size_type capacity) : base_type(capacity) {}
		explicit ValueHistory(size_type capacity, allocator_type const& allocator) : base_type(capacity, allocator) {}
		explicit ValueHistory(size_type size, T const& fill_value) : base_type(size) {
			fill(fill_value);
		}

		void fill(T const& value) {
			base_type::resize(base_type::capacity(), value);
		}
	};

	template<typename T>
	std::ostream& operator<<(std::ostream& stream, ValueHistory<T> const& history) {
		stream << "[";

		bool add_comma = false;

		for (T const& value : history) {
			if (add_comma) {
				stream << ", ";
			} else {
				add_comma = true;
			}

			stream << value;
		}

		return stream << "]";
	}
}
