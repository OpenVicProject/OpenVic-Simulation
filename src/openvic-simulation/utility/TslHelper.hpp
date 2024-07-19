#pragma once

#include <type_traits>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	template<IsOrderedMap Map, typename Key = typename Map::key_type, typename Mapped = typename Map::mapped_type>
	struct _OrderedMapMutable {
		using map_type = Map;
		struct ordered_iterator {
			using key_type = Key;
			using mapped_type = Mapped;
			using pair_type = std::pair<key_type const&, mapped_type&>;
			using value_type = pair_type;
			using iterator = typename map_type::iterator;

			using iterator_category = std::random_access_iterator_tag;
			using difference_type = typename map_type::values_container_type::iterator::difference_type;

			pair_type operator*() {
				return { m_iterator.key(), m_iterator.value() };
			}

			ordered_iterator& operator++() {
				++m_iterator;
				return *this;
			}
			ordered_iterator& operator--() {
				--m_iterator;
				return *this;
			}

			ordered_iterator operator++(int) {
				ordered_iterator tmp(*this);
				++(*this);
				return tmp;
			}
			ordered_iterator operator--(int) {
				ordered_iterator tmp(*this);
				--(*this);
				return tmp;
			}

			pair_type operator[](difference_type n) {
				return *(*this + n);
			}

			ordered_iterator& operator+=(difference_type n) {
				m_iterator += n;
				return *this;
			}
			ordered_iterator& operator-=(difference_type n) {
				m_iterator -= n;
				return *this;
			}

			ordered_iterator operator+(difference_type n) {
				ordered_iterator tmp(*this);
				tmp += n;
				return tmp;
			}
			ordered_iterator operator-(difference_type n) {
				ordered_iterator tmp(*this);
				tmp -= n;
				return tmp;
			}

			bool operator==(ordered_iterator const& rhs) const {
				return m_iterator == rhs.m_iterator;
			}

			bool operator!=(ordered_iterator const& rhs) const {
				return m_iterator != rhs.m_iterator;
			}

			bool operator<(ordered_iterator const& rhs) const {
				return m_iterator < rhs.m_iterator;
			}

			bool operator>(ordered_iterator const& rhs) const {
				return m_iterator > rhs.m_iterator;
			}

			bool operator<=(ordered_iterator const& rhs) const {
				return m_iterator <= rhs.m_iterator;
			}

			bool operator>=(ordered_iterator const& rhs) const {
				return m_iterator >= rhs.m_iterator;
			}

			friend ordered_iterator operator+(difference_type n, ordered_iterator const& it) {
				return n + it.m_iterator;
			}

			ordered_iterator operator+(ordered_iterator const& rhs) const {
				return m_iterator + rhs.m_iterator;
			}

			difference_type operator-(ordered_iterator const& rhs) const {
				return m_iterator - rhs.m_iterator;
			}

			iterator m_iterator;
		};

		_OrderedMapMutable(map_type& map) : _map(map) {}

		ordered_iterator begin() {
			return ordered_iterator { _map.begin() };
		}
		ordered_iterator end() {
			return ordered_iterator { _map.end() };
		}

	private:
		map_type& _map;
	};

	template<IsOrderedMap Map, typename Key = typename Map::key_type, typename Mapped = typename Map::mapped_type>
	_OrderedMapMutable<Map, Key, Mapped> mutable_iterator(Map& map) {
		return _OrderedMapMutable<Map, Key, Mapped> { map };
	}
}
