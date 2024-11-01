#pragma once

#include <concepts>
#include <vector>

#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {

	template<typename Key, typename Value>
	struct IndexedMap : private std::vector<Value> {
		using container_t = std::vector<Value>;
		using key_t = Key;
		using value_t = Value;
		using value_ref_t = container_t::reference;
		using value_const_ref_t = container_t::const_reference;
		using keys_t = std::vector<key_t>;

		using key_type = key_t; // To match tsl::ordered_map's key_type
		using mapped_type = value_t; // To match tsl::ordered_map's mapped_type

		using container_t::operator[];
		using container_t::size;
		using container_t::begin;
		using container_t::end;

	private:
		keys_t const* PROPERTY(keys);

	public:
		constexpr IndexedMap(keys_t const* new_keys) : keys { nullptr } {
			set_keys(new_keys);
		}

		IndexedMap(IndexedMap const&) = default;
		IndexedMap(IndexedMap&&) = default;
		IndexedMap& operator=(IndexedMap const&) = default;
		IndexedMap& operator=(IndexedMap&&) = default;

		constexpr void fill(value_t const& value) {
			std::fill(container_t::begin(), container_t::end(), value);
		}

		constexpr void clear() {
			fill({});
		}

		constexpr bool has_keys() const {
			return keys != nullptr;
		}

		constexpr void set_keys(keys_t const* new_keys) {
			if (keys != new_keys) {
				keys = new_keys;

				container_t::resize(keys != nullptr ? keys->size() : 0);
				clear();
			}
		}

		constexpr size_t get_index_from_item(key_t const& key) const {
			if (has_keys() && keys->data() <= &key && &key <= &keys->back()) {
				return std::distance(keys->data(), &key);
			} else {
				return 0;
			}
		}

		constexpr value_t* get_item_by_key(key_t const& key) {
			const size_t index = get_index_from_item(key);
			if (index < container_t::size()) {
				return &container_t::operator[](index);
			}
			return nullptr;
		}

		constexpr value_t const* get_item_by_key(key_t const& key) const {
			const size_t index = get_index_from_item(key);
			if (index < container_t::size()) {
				return &container_t::operator[](index);
			}
			return nullptr;
		}

		constexpr key_t const& operator()(size_t index) const {
			return (*keys)[index];
		}

		constexpr key_t const* get_key_by_index(size_t index) const {
			if (keys != nullptr && index < keys->size()) {
				return &(*this)(index);
			} else {
				return nullptr;
			}
		}

		constexpr value_ref_t operator[](key_t const& key) {
			return container_t::operator[](get_index_from_item(key));
		}

		constexpr value_const_ref_t operator[](key_t const& key) const {
			return container_t::operator[](get_index_from_item(key));
		}

		constexpr IndexedMap& operator+=(IndexedMap const& other) {
			const size_t count = std::min(container_t::size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				container_t::operator[](index) += other[index];
			}
			return *this;
		}

		constexpr IndexedMap& operator*=(value_t factor) {
			for (value_t& value : *this) {
				value *= factor;
			}
			return *this;
		}

		constexpr IndexedMap& operator/=(value_t divisor) {
			for (value_t& value : *this) {
				value /= divisor;
			}
			return *this;
		}

		constexpr IndexedMap operator+(IndexedMap const& other) const {
			IndexedMap ret = *this;
			ret += other;
			return ret;
		}

		constexpr IndexedMap operator*(value_t factor) const {
			IndexedMap ret = *this;
			ret *= factor;
			return ret;
		}

		constexpr IndexedMap operator/(value_t divisor) const {
			IndexedMap ret = *this;
			ret /= divisor;
			return ret;
		}

		constexpr value_t get_total() const {
			value_t total {};
			for (value_t const& value : *this) {
				total += value;
			}
			return total;
		}

		constexpr IndexedMap& normalise() {
			const value_t total = get_total();
			if (total > 0) {
				*this /= total;
			}
			return *this;
		}

		constexpr bool copy(IndexedMap const& other) {
			if (keys != other.keys) {
				Logger::error(
					"Trying to copy IndexedMaps with different keys with sizes: from ", other.size(), " to ",
					container_t::size()
				);
				return false;
			}
			static_cast<container_t&>(*this) = other;
			return true;
		}

		constexpr void write_non_empty_values(IndexedMap const& other) {
			const size_t count = std::min(container_t::size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				value_t const& value = other[index];
				if (value) {
					container_t::operator[](index) = value;
				}
			}
		}

		fixed_point_map_t<key_t const *> to_fixed_point_map() const
		requires(std::same_as<value_t, fixed_point_t>)
		{
			fixed_point_map_t<key_t const *> result;

			for (size_t index = 0; index < container_t::size(); index++) {
				fixed_point_t const& value = container_t::operator[](index);
				if (value != 0) {
					result[&(*this)(index)] = value;
				}
			}

			return result;
		}
	};

	template<typename FPMKey, std::derived_from<FPMKey> IMKey>
	constexpr fixed_point_map_t<FPMKey const*>& operator+=(
		fixed_point_map_t<FPMKey const*>& lhs, IndexedMap<IMKey, fixed_point_t> const& rhs
	) {
		for (size_t index = 0; index < rhs.size(); index++) {
			fixed_point_t const& value = rhs[index];
			if (value != 0) {
				lhs[&rhs(index)] += value;
			}
		}
		return lhs;
	}

	/* Result is determined by comparing the first pair of unequal values,
	 * iterating from the highest index downward. */
	template<typename Key, typename Value>
	constexpr bool sorted_indexed_map_less_than(
		IndexedMap<Key, Value> const& lhs, IndexedMap<Key, Value> const& rhs
	) {
		if (lhs.get_keys() != rhs.get_keys() || lhs.size() != rhs.size()) {
			Logger::error("Trying to compare IndexedMaps with different keys/sizes: ", lhs.size(), " vs ", rhs.size());
			return false;
		}

		for (size_t index = lhs.size(); index > 0;) {
			index--;
			const std::strong_ordering value_cmp = lhs[index] <=> rhs[index];
			if (value_cmp != std::strong_ordering::equal) {
				return value_cmp == std::strong_ordering::less;
			}
		}

		return false;
	}
}
