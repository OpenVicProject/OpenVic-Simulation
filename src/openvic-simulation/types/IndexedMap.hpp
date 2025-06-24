#pragma once

#include <concepts>
#include <vector>

#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	namespace _detail::IndexedMap {
		// Hides most vector methods, makes resize protected, only exposes accessor methods
		template<utility::not_same_as<bool> Value>
		struct internal_resize_vector : private std::vector<Value> {
			using base_type = std::vector<Value>;

			using typename base_type::const_iterator;
			using typename base_type::const_pointer;
			using typename base_type::const_reference;
			using typename base_type::iterator;
			using typename base_type::pointer;
			using typename base_type::reference;
			using base_type::operator[];
			using base_type::back;
			using base_type::begin;
			using base_type::cbegin;
			using base_type::cend;
			using base_type::data;
			using base_type::end;
			using base_type::front;
			using base_type::size;

		protected:
			using base_type::resize;
		};
	}

	template<utility::not_same_as<bool> Key, utility::not_same_as<bool> Value>
	struct IndexedMap : public _detail::IndexedMap::internal_resize_vector<Value> {
		using values_type = _detail::IndexedMap::internal_resize_vector<Value>;

		using values_type::operator[];
		using values_type::data;

		using key_type = Key;
		using keys_type = std::vector<key_type>;
		using keys_span_type = OpenVic::utility::forwardable_span<const key_type>;
		using key_ref_type = typename keys_type::const_reference;
		using key_ptr_type = typename keys_type::const_pointer;
		using key_iterator = typename keys_span_type::iterator;

		using value_type = Value;
		using value_ref_type = typename values_type::reference;
		using value_const_ref_type = typename values_type::const_reference;
		using value_ptr_type = typename values_type::pointer;
		using value_const_ptr_type = typename values_type::const_pointer;
		using value_iterator = typename values_type::iterator;
		using value_const_iterator = typename values_type::const_iterator;
		using mapped_type = value_type; // To match tsl::ordered_map's mapped_type

		using pair_type = std::pair<key_ref_type, value_ref_type>;
		using const_pair_type = std::pair<key_ref_type, value_const_ref_type>;

		template<bool CONST>
		struct base_iterator_t {
			using map_type = IndexedMap<Key, Value>;

			using key_type = map_type::key_type;
			using key_ref_type = map_type::key_ref_type;

			using mapped_type = map_type::value_type;
			using mapped_ref_type = std::conditional_t<CONST, map_type::value_const_ref_type, map_type::value_ref_type>;

			using pair_type = std::pair<key_ref_type, mapped_ref_type>;
			using value_type = pair_type;

			using key_iterator = map_type::key_iterator;
			using value_iterator = typename std::conditional_t<CONST, map_type::value_const_iterator, map_type::value_iterator>;

			using iterator_category = std::random_access_iterator_tag;
			using difference_type = typename map_type::keys_type::difference_type;

		private:
			key_iterator PROPERTY(key_it);
			value_iterator PROPERTY(value_it);

		public:
			base_iterator_t() = default;

			base_iterator_t(key_iterator new_key_it, value_iterator new_value_it)
				: key_it { new_key_it }, value_it { new_value_it } {}

			operator base_iterator_t<true>() const {
				return { key_it, value_it };
			}

			pair_type operator*() const {
				return { *key_it, *value_it };
			}

			key_ref_type key() const {
				return *key_it;
			}

			mapped_ref_type value() const {
				return *value_it;
			}

			base_iterator_t operator++() {
				++key_it;
				++value_it;
				return *this;
			}

			base_iterator_t operator--() {
				--key_it;
				--value_it;
				return *this;
			}

			base_iterator_t operator++(int) {
				base_iterator_t tmp { *this };
				++(*this);
				return tmp;
			}

			base_iterator_t operator--(int) {
				base_iterator_t tmp { *this };
				--(*this);
				return tmp;
			}

			pair_type operator[](difference_type n) const {
				return *(*this + n);
			}

			base_iterator_t& operator+=(difference_type n) {
				key_it += n;
				value_it += n;
				return *this;
			}

			base_iterator_t& operator-=(difference_type n) {
				key_it -= n;
				value_it -= n;
				return *this;
			}

			base_iterator_t operator+(difference_type n) const {
				base_iterator_t tmp { *this };
				tmp += n;
				return tmp;
			}

			base_iterator_t operator-(difference_type n) const {
				base_iterator_t tmp { *this };
				tmp -= n;
				return tmp;
			}

			friend base_iterator_t operator+(difference_type lhs, base_iterator_t const& rhs) {
				return rhs + lhs;
			}

			// We only need to compare one of the iterators for the following operations as the
			// key and value iterators will always be the same position in their respective vectors.

			template<bool OTHER_CONST>
			difference_type operator-(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it - rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator==(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it == rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator!=(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it != rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator<(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it < rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator>(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it > rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator<=(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it <= rhs.get_key_it();
			}

			template<bool OTHER_CONST>
			bool operator>=(base_iterator_t<OTHER_CONST> const& rhs) const {
				return key_it >= rhs.get_key_it();
			}
		};

		using iterator = base_iterator_t<false>;
		using const_iterator = base_iterator_t<true>;

	private:
		keys_span_type PROPERTY(keys, {});

	public:
		constexpr IndexedMap() = default;

		constexpr IndexedMap(keys_span_type new_keys) : IndexedMap() {
			set_keys(new_keys);
		}

		IndexedMap(IndexedMap const&) = default;
		IndexedMap(IndexedMap&&) = default;
		IndexedMap& operator=(IndexedMap const&) = default;
		IndexedMap& operator=(IndexedMap&&) = default;

		values_type& get_values() {
			return *this;
		}

		values_type const& get_values() const {
			return *this;
		}

		iterator begin() {
			if (has_keys()) {
				return { keys.begin(), get_values().begin() };
			} else {
				return {};
			}
		}

		iterator end() {
			if (has_keys()) {
				return { keys.end(), get_values().end() };
			} else {
				return {};
			}
		}

		const_iterator cbegin() const {
			if (has_keys()) {
				return { keys.begin(), get_values().cbegin() };
			} else {
				return {};
			}
		}

		const_iterator cend() const {
			if (has_keys()) {
				return { keys.end(), get_values().cend() };
			} else {
				return {};
			}
		}

		const_iterator begin() const {
			return cbegin();
		}

		const_iterator end() const {
			return cend();
		}

		pair_type front() {
			return { keys.front(), get_values().front() };
		}

		const_pair_type front() const {
			return { keys.front(), get_values().front() };
		}

		pair_type back() {
			return { keys.back(), get_values().back() };
		}

		const_pair_type back() const {
			return { keys.back(), get_values().back() };
		}

		constexpr void fill(value_const_ref_type value) {
			std::fill(get_values().begin(), get_values().end(), value);
		}

		constexpr void clear() {
			for (value_ref_type value : get_values()) {
				value = Value {};
			}
		}

		constexpr bool has_keys() const {
			return !keys.empty();
		}

		constexpr void set_keys(keys_span_type new_keys) {
			if (keys.data() != new_keys.data()) {
				keys = new_keys;

				values_type::resize(has_keys() ? keys.size() : 0);
				clear();
			}
		}

		constexpr size_t get_index_from_item(key_ref_type key) const {
			if (has_keys() && keys.data() <= &key && &key <= &keys.back()) {
				return std::distance(keys.data(), &key);
			} else {
				return 0;
			}
		}

		constexpr value_ptr_type get_item_by_key(key_ref_type key) {
			const size_t index = get_index_from_item(key);
			if (index < get_values().size()) {
				return &get_values()[index];
			} else {
				return nullptr;
			}
		}

		constexpr value_const_ptr_type get_item_by_key(key_ref_type key) const {
			const size_t index = get_index_from_item(key);
			if (index < get_values().size()) {
				return &get_values()[index];
			} else {
				return nullptr;
			}
		}

		constexpr key_ref_type operator()(size_t index) const {
			return keys[index];
		}

		constexpr key_ptr_type get_key_by_index(size_t index) const {
			if (has_keys() && index < keys.size()) {
				return &(*this)(index);
			} else {
				return nullptr;
			}
		}

		constexpr value_ref_type operator[](key_ref_type key) {
			return get_values()[get_index_from_item(key)];
		}

		constexpr value_const_ref_type operator[](key_ref_type key) const {
			return get_values()[get_index_from_item(key)];
		}

		constexpr IndexedMap& operator+=(IndexedMap const& other) {
			const size_t count = std::min(get_values().size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				get_values()[index] += other[index];
			}
			return *this;
		}

		constexpr IndexedMap& mul_add(IndexedMap const& other, value_type factor) {
			const size_t count = std::min(get_values().size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				get_values()[index] += other[index] * factor;
			}
			return *this;
		}

		template<typename OtherValue1, typename OtherValue2>
		constexpr IndexedMap& mul_add(IndexedMap<Key, OtherValue1> const& other1, IndexedMap<Key, OtherValue2> const& other2) {
			const size_t count = std::min(get_values().size(), std::min(other1.size(), other2.size()));
			for (size_t index = 0; index < count; ++index) {
				get_values()[index] += other1[index] * other2[index];
			}
			return *this;
		}

		constexpr IndexedMap& operator*=(value_type factor) {
			for (value_ref_type value : get_values()) {
				value *= factor;
			}
			return *this;
		}

		constexpr IndexedMap& operator/=(value_type divisor) {
			for (value_ref_type value : get_values()) {
				value /= divisor;
			}
			return *this;
		}

		template<typename OtherValue>
		constexpr IndexedMap& operator/=(IndexedMap<Key, OtherValue> const& other) {
			const size_t count = std::min(get_values().size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				value_type const& other_value = other[index];
				if (other_value != OtherValue {}) {
					get_values()[index] /= other_value;
				}
			}
			return *this;
		}

		constexpr IndexedMap operator+(IndexedMap const& other) const {
			IndexedMap ret = *this;
			ret += other;
			return ret;
		}

		constexpr IndexedMap operator*(value_type factor) const {
			IndexedMap ret = *this;
			ret *= factor;
			return ret;
		}

		constexpr IndexedMap operator/(value_type divisor) const {
			IndexedMap ret = *this;
			ret /= divisor;
			return ret;
		}

		constexpr value_type get_total() const {
			value_type total {};
			for (value_const_ref_type value : get_values()) {
				total += value;
			}
			return total;
		}

		constexpr IndexedMap& normalise() {
			const value_type total = get_total();
			if (total > 0) {
				*this /= total;
			}
			return *this;
		}

		constexpr IndexedMap& rescale(fixed_point_t new_total) {
			const value_type total = get_total();
			if (total > 0) {
				for (value_ref_type value : get_values()) {
					value *= new_total;
					value /= total;
				}
			}
			return *this;
		}

		constexpr bool copy(IndexedMap const& other) {
			if (keys.data() != other.keys.data()) {
				Logger::error(
					"Trying to copy IndexedMaps with different keys with sizes: from ", other.size(), " to ",
					get_values().size()
				);
				return false;
			}
			get_values() = other;
			return true;
		}

		constexpr void write_non_empty_values(IndexedMap const& other) {
			const size_t count = std::min(get_values().size(), other.size());
			for (size_t index = 0; index < count; ++index) {
				value_const_ref_type value = other[index];
				if (value) {
					get_values()[index] = value;
				}
			}
		}

		fixed_point_map_t<key_ptr_type> to_fixed_point_map() const
		requires(std::same_as<value_type, fixed_point_t>)
		{
			fixed_point_map_t<key_ptr_type> result;

			for (size_t index = 0; index < get_values().size(); index++) {
				value_const_ref_type value = get_values()[index];
				if (value != 0) {
					result[&(*this)(index)] = value;
				}
			}

			return result;
		}
	};

	template<typename FPMKey, std::derived_from<FPMKey> IMKey>
	constexpr fixed_point_map_t<FPMKey const*>&
	operator+=(fixed_point_map_t<FPMKey const*>& lhs, IndexedMap<IMKey, fixed_point_t> const& rhs) {
		for (size_t index = 0; index < rhs.size(); index++) {
			typename IndexedMap<IMKey, fixed_point_t>::value_const_ref_type value = rhs[index];
			if (value != 0) {
				lhs[&rhs(index)] += value;
			}
		}
		return lhs;
	}

	/* Result is determined by comparing the first pair of unequal values,
	 * iterating from the highest index downward. */
	template<typename Key, typename Value>
	constexpr bool sorted_indexed_map_less_than(IndexedMap<Key, Value> const& lhs, IndexedMap<Key, Value> const& rhs) {
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
