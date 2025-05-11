#pragma once

#include <numeric>
#include <ostream>
#include <vector>

#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	template<typename T>
	struct ValueHistory : private memory::vector<T> {
		using base_type = memory::vector<T>;

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
		using base_type::data;
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

		ValueHistory() = default;
		ValueHistory(size_t history_size, T const& fill_value = {}) : base_type(history_size, fill_value) {}
		ValueHistory(ValueHistory&&) = default;

		void fill(T const& value) {
			std::fill(begin(), end(), value);
		}

		T get_total() const {
			return std::accumulate(begin(), end(), T {});
		}

		void push_back(T const& value) {
			if (!empty()) {
				// Move all values back by one (with the first value being discarded) and place the new value at the end
				for (iterator it = begin(); it != end(); ++it) {
					*it = *(it + 1);
				}
				back() = value;
			}
		}

		void set_size(size_t new_size, T const& fill_value = {}) {
			const difference_type size_diff = new_size - size();

			if (size_diff < 0) {
				// Move the last new_size elements to the front, discarding the values before them
				for (iterator it = begin(); it != begin() + new_size; ++it) {
					*it = *(it - size_diff);
				}

				base_type::resize(new_size);
			} else if (size_diff > 0) {
				// Move the existing values to the end and fill the front with fill_value
				const size_t old_size = size();
				base_type::resize(new_size);

				for (reverse_iterator rit = rbegin(); rit != rbegin() + old_size; ++rit) {
					*rit = *(rit + size_diff);
				}
				for (iterator it = begin(); it != begin() + size_diff; ++it) {
					*it = fill_value;
				}
			}
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
