#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>

#include <function2/function2.hpp>

#include "openvic-simulation/types/FixedVector.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"
#include "openvic-simulation/utility/MathConcepts.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	template <typename T>
	concept HasGetIndex = requires(T const& key) {
		{ key.get_index() } -> std::convertible_to<size_t>;
	};

	/**
	* @brief A dictionary-like type that uses std::vector for contiguous storage,
	* providing O(1) access time by directly using an integer index obtained from the KeyType.
	* It strictly assumes that keys provided at construction are ordered and continuous
	* in their index values.
	* @tparam KeyType The type of keys used to access values. This type must provide a
	* `size_t getIndex() const` method.
	* @tparam ValueType The type of values to be stored in the map.
	*
	* This class assumes that an integer index can be obtained from the key type
	* via its `getIndex()` method. The indices used with this map must be within
	* the specified range [min_index, max_index].
	*
	* @warning This class stores a `std::span` of the provided keys. This means the
	* `IndexedFlatMap` does NOT own the lifetime of the keys. The caller is
	* responsible for ensuring that the underlying data (the `std::vector` or array)
	* that the `std::span` refers to remains valid and outlives the `IndexedFlatMap` instance.
	*/
	template <typename KeyType, typename ValueType>
	struct IndexedFlatMap {
		using keys_span_type = OpenVic::utility::forwardable_span<const KeyType>;
		using values_vector_type = std::conditional_t<
            std::is_move_constructible_v<ValueType> || std::is_copy_constructible_v<ValueType>,
            memory::vector<ValueType>,
            memory::FixedVector<ValueType>
        >;

	private:
		values_vector_type values;
		keys_span_type keys; //non-owning!
		size_t min_index;
		size_t max_index;

		/**
		* @brief Converts an external key's index to an internal vector index.
		* Logs a fatal error if the key's index is out of bounds.
		* @param key The key whose index is to be converted.
		* @return The internal index, or 0 if out of bounds (after logging error).
		*/
		constexpr size_t get_internal_index_from_key(KeyType const& key) const {
			static_assert(HasGetIndex<KeyType>);
			const size_t index = key.get_index();
			if (index < min_index || index > max_index) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> attempted to access key with index {} which is outside the map's defined range [{}, {}].",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					index,
					min_index,
					max_index
				);
				assert(index >= min_index && index <= max_index);
				return 0;
			}
			return index - min_index;
		}

		/**
		* @brief Validates that the provided span of keys is ordered and continuous.
		* Logs errors if validation fails.
		* @param new_keys The span of keys to validate.
		* @return True if keys are valid, false otherwise.
		*/
		static bool validate_new_keys(keys_span_type new_keys) {
			static_assert(HasGetIndex<KeyType>);
			if (new_keys.empty()) {
				spdlog::warn_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{}, {}> should not be constructed with empty key span.",
					utility::type_name<KeyType>(), utility::type_name<ValueType>()
				);
				return false;
			}

			const size_t min_index = new_keys.front().get_index();
			const size_t max_index = new_keys.back().get_index();
			const size_t expected_capacity = max_index - min_index + 1;

			if (new_keys.size() != expected_capacity) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> must be constructed with a continuous span of keys with incremental indices. Expected capacity {} but got {} keys.",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					expected_capacity,
					new_keys.size()
				);
				assert(new_keys.size() == expected_capacity);
				return false;
			}

			for (size_t i = 0; i < new_keys.size(); ++i) {
				if (new_keys[i].get_index() != min_index + i) {
					spdlog::error_s(
						"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> must be constructed with a continuous span of keys with incremental indices. "
						"Expected index {} but got {} at position {}.",
						utility::type_name<KeyType>(),
						utility::type_name<ValueType>(),
						min_index + i,
						new_keys[i].get_index(),
						i
					);
					assert(new_keys[i].get_index() == min_index + i);
					return false;
				}
			}

			return true;
		}

		//could be rewritten to return iterators for both this and other.
		//that would overcomplicate it with const & non-const
		template <typename OtherValueType>
		keys_span_type get_shared_keys(IndexedFlatMap<KeyType,OtherValueType> const& other) const {
			if (other.get_min_index() >= min_index && other.get_max_index() <= max_index) {
				return other.get_keys();
			}

			const size_t min_shared_index = std::max(other.get_min_index(), min_index);
			const size_t max_shared_index = std::min(other.get_max_index(), max_index);

			if (min_shared_index > max_shared_index) {
				return {};
			}

			const size_t shared_count = 1 + max_shared_index - min_shared_index;
			const size_t other_keys_offset = min_shared_index - other.get_min_index();
			return {other.get_keys().data() + other_keys_offset, shared_count};
		}

		/**
		* @brief Checks if the 'other' IndexedFlatMap's key span is a subset of 'this' map's key span,
		* based purely on index ranges.
		* Logs a fatal error if not compatible.
		* @param other The other IndexedFlatMap to compare with.
		* @return True if compatible as a subset, false otherwise.
		*/
		template <typename OtherValueType>
		bool check_subset_span_match(IndexedFlatMap<KeyType,OtherValueType> const& other) const {
			// Check if 'other's index range is contained within 'this's index range
			if (!(other.get_min_index() >= min_index && other.get_max_index() <= max_index)) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> subset operation requires the right-hand map's index range "
					"({}-{}) to be a subset of the left-hand map's index range ({}-{}).",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					other.get_min_index(),
					other.get_max_index(),
					min_index,
					max_index
				);
				assert(other.get_min_index() >= min_index && other.get_max_index() <= max_index);
				return false;
			}

			// There is no check for keys.data() being identical as KeyType objects are considered functionally equivalent if their get_index() values match.
			return true;
		}
		
		//vector
		constexpr IndexedFlatMap()
		requires std::is_move_constructible_v<ValueType> || std::is_copy_constructible_v<ValueType>
		: values(), keys(), min_index(0), max_index(0) {}

		//FixedVector
		constexpr IndexedFlatMap()
		requires (!std::is_move_constructible_v<ValueType>) && (!std::is_copy_constructible_v<ValueType>)
		: values(0), keys(), min_index(0), max_index(0) {}

	public:
		static constexpr IndexedFlatMap create_empty() {
			return {};
		}

		/** vector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys
		* and a key-based generator.
		* The map's range [min_idx, max_idx] is determined by the first and last key in the span.
		* All elements are initialized using the `value_generator`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		* @param value_generator A callable that takes a `KeyType const&` and returns a single argument to construct `ValueType`.
		* This is used to generate values for each key in the provided span.
		*/
		template<typename GeneratorTemplateType>
		requires (std::is_move_constructible_v<ValueType> || std::is_copy_constructible_v<ValueType>)
		//value_generator(key) doesn't return std:tuple<...>
		&& (!utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, KeyType const&>>, std::tuple>)
		//ValueType(value_generator(key)) is valid constructor call
		&& std::constructible_from<ValueType, decltype(std::declval<GeneratorTemplateType>()(std::declval<KeyType const&>()))>
		IndexedFlatMap(
			keys_span_type new_keys,
			GeneratorTemplateType value_generator
		) : keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values() {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			values.reserve(new_keys.size());
			for (KeyType const& key : keys) {
				values.emplace_back(
					std::forward<decltype(std::declval<GeneratorTemplateType>()(std::declval<KeyType const&>()))>(
						value_generator(key)
					)
				);
			}
		}

		/** FixedVector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys
		* and a key-based generator.
		* The map's range [min_idx, max_idx] is determined by the first and last key in the span.
		* All elements are initialized using the `value_generator`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		* @param value_generator A callable that takes a `KeyType const&` and returns a single argument to construct `ValueType`.
		* This is used to generate values for each key in the provided span.
		*/
		template<typename GeneratorTemplateType>
		requires (!std::is_move_constructible_v<ValueType>) && (!std::is_copy_constructible_v<ValueType>)
		//value_generator(key) doesn't return std:tuple<...>
		&& (!utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, KeyType const&>>, std::tuple>)
		//ValueType(value_generator(key)) is valid constructor call
		&& std::constructible_from<ValueType, decltype(std::declval<GeneratorTemplateType>()(std::declval<KeyType const&>()))>
		IndexedFlatMap(
			keys_span_type new_keys,
			GeneratorTemplateType value_generator
		) : keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values(new_keys.size()) {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			for (KeyType const& key : keys) {
				values.emplace_back(
					std::forward<decltype(std::declval<GeneratorTemplateType>()(std::declval<KeyType const&>()))>(
						value_generator(key)
					)
				);
			}
		}

		/** vector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys
		* and a key-based generator.
		* The map's range [min_idx, max_idx] is determined by the first and last key in the span.
		* All elements are initialized using the `value_generator`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		* @param value_generator A callable that takes a `KeyType const&` and returns arguments to construct `ValueType`.
		* This is used to generate values for each key in the provided span.
		*/
		template<typename GeneratorTemplateType>
		requires (std::is_move_constructible_v<ValueType> || std::is_copy_constructible_v<ValueType>)
		//value_generator(key) returns tuple
		&& utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, KeyType const&>>, std::tuple>
		//ValueType(...value_generator(key)) is valid constructor call
		&& requires(GeneratorTemplateType&& value_generator) { {
			std::apply(
				[](auto&&... args) {
					ValueType obj{std::forward<decltype(args)>(args)...};
				},
				value_generator(std::declval<KeyType const&>())
			)
		}; }
		IndexedFlatMap(
			keys_span_type new_keys,
			GeneratorTemplateType&& value_generator
		) : keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values() {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			values.reserve(new_keys.size());
			for (KeyType const& key : keys) {
				std::apply(
					[this](auto&&... args) {
						values.emplace_back(std::forward<decltype(args)>(args)...);
					},
					value_generator(key)
				);
			}
		}

		/** FixedVector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys
		* and a key-based generator.
		* The map's range [min_idx, max_idx] is determined by the first and last key in the span.
		* All elements are initialized using the `value_generator`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		* @param value_generator A callable that takes a `KeyType const&` and returns arguments to construct `ValueType`.
		* This is used to generate values for each key in the provided span.
		*/
		template<typename GeneratorTemplateType>
		requires (!std::is_move_constructible_v<ValueType>) && (!std::is_copy_constructible_v<ValueType>)
		//value_generator(key) returns tuple
		&& utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, KeyType const&>>, std::tuple>
		//ValueType(...value_generator(key)) is valid constructor call
		&& requires(GeneratorTemplateType value_generator) { {
			std::apply(
				[](auto&&... args) {
					ValueType obj{std::forward<decltype(args)>(args)...};
				},
				value_generator(std::declval<KeyType const&>())
			)
		}; }
		IndexedFlatMap(
			keys_span_type new_keys,
			GeneratorTemplateType value_generator
		) : keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values(new_keys.size()) {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			for (KeyType const& key : keys) {
				std::apply(
					[this](auto&&... args) {
						values.emplace_back(std::forward<decltype(args)>(args)...);
					},
					value_generator(key)
				);
			}
		}

		/** vector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys.
		* All elements are constructed by passing `KeyType const&` or default-constructed if there is no constructor taking `KeyType const&`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		*
		* @note This constructor requires `ValueType` to be `std::default_initializable || std::constructible_from<ValueType, KeyType const&>`.
		*/
		IndexedFlatMap(keys_span_type new_keys)
		requires (std::is_move_constructible_v<ValueType> || std::is_copy_constructible_v<ValueType>)
		&& (std::default_initializable<ValueType> || std::constructible_from<ValueType, KeyType const&>)
			: keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values() {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			if constexpr (std::constructible_from<ValueType, KeyType const&>) {
				values.reserve(new_keys.size());
				for (KeyType const& key : keys) {
					values.emplace_back(key);
				}
			} else {
				// Resize and default-construct elements
				values.resize(new_keys.size());
			}
		}

		/** FixedVector
		* @brief Constructs an IndexedFlatMap based on a provided span of ordered and continuous keys.
		* All elements are constructed by passing `KeyType const&` or default-constructed if there is no constructor taking `KeyType const&`.
		* @param new_keys A `std::span<const KeyType>` of KeyType objects. These keys MUST be
		* ordered by their index and continuous (i.e., `new_keys[i+1].getIndex() == new_keys[i].getIndex() + 1`).
		* The `IndexedFlatMap` stores a reference to this span; the caller is responsible
		* for ensuring the underlying data outlives this map instance.
		*
		* @note This constructor requires `ValueType` to be `std::default_initializable || std::constructible_from<ValueType, KeyType const&>`.
		*/
		IndexedFlatMap(keys_span_type new_keys)
		requires (!std::is_move_constructible_v<ValueType>) && (!std::is_copy_constructible_v<ValueType>)
		&& (std::default_initializable<ValueType> || std::constructible_from<ValueType, KeyType const&>)
			: keys(new_keys),
			min_index { new_keys.front().get_index() },
			max_index { new_keys.back().get_index() },
			values(new_keys.size()) {
			static_assert(HasGetIndex<KeyType>);
			if (!validate_new_keys(new_keys)) {
				return;
			}

			if constexpr (std::constructible_from<ValueType, KeyType const&>) {
				for (KeyType const& key : keys) {
					values.emplace_back(key);
				}
			} else {
				// Resize and default-construct elements
				for (KeyType const& key : keys) {
					values.emplace_back();
				}
			}
		}

		/**
		* @brief Sets the value associated with a key using copy assignment.
		*/
		void set(KeyType const& key, ValueType const& value)
		requires std::assignable_from<ValueType&, ValueType const&> {
			static_assert(HasGetIndex<KeyType>);
			values[get_internal_index_from_key(key)] = value;
		}

		/**
		* @brief Sets the value associated with a key using move assignment (via std::swap).
		*/
		void set(KeyType const& key, ValueType&& value)
		requires std::movable<ValueType> {
			static_assert(HasGetIndex<KeyType>);
			std::swap(
				values[get_internal_index_from_key(key)],
				value
			);
		}

		constexpr ValueType& at(KeyType const& key) {
			static_assert(HasGetIndex<KeyType>);
			return values[get_internal_index_from_key(key)];
		}

		constexpr ValueType const& at(KeyType const& key) const {
			static_assert(HasGetIndex<KeyType>);
			return values[get_internal_index_from_key(key)];
		}

		constexpr ValueType& at_index(const size_t index) {
			if (index < min_index || index > max_index) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> attempted to access index {} which is outside the map's defined range [{}, {}].",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					index,
					min_index,
					max_index
				);
				assert(index >= min_index && index <= max_index);
			}
			return values[index - min_index];
		}

		constexpr ValueType const& at_index(const size_t index) const {
			if (index < min_index || index > max_index) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> attempted to access index {} which is outside the map's defined range [{}, {}].",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					index,
					min_index,
					max_index
				);
				assert(index >= min_index && index <= max_index);
			}
			return values[index - min_index];
		}

		constexpr bool contains(KeyType const& key) const {
			static_assert(HasGetIndex<KeyType>);
			return contains_index(key.get_index());
		}

		constexpr bool contains_index(const size_t external_index) const {
			return external_index >= min_index && external_index <= max_index;
		}

		constexpr keys_span_type const& get_keys() const {
			return keys;
		}
		
		constexpr OpenVic::utility::forwardable_span<ValueType> get_values() {
			return values;
		}

		constexpr OpenVic::utility::forwardable_span<const ValueType> get_values() const {
			return values;
		}

		constexpr size_t get_count() const {
			return keys.size();
		}

		constexpr bool empty() const {
			return keys.empty();
		}

		constexpr size_t get_min_index() const {
			return min_index;
		}

		constexpr size_t get_max_index() const {
			return max_index;
		}

		/**
		* @brief Fills all elements in the map with a specified value.
		* The capacity and index range remain unchanged.
		* @param value The value to fill all elements with.
		*
		* @note This method requires `ValueType` to be copy-constructible and copy-assignable.
		*/
		void fill(ValueType const& value)
		requires std::copy_constructible<ValueType> && std::assignable_from<ValueType&, ValueType const&> {
			values.assign(values.size(), value);
		}

		constexpr void copy_values_from(IndexedFlatMap const& other)
		requires std::assignable_from<ValueType&, ValueType const&> {
			static_assert(HasGetIndex<KeyType>);
			for (KeyType const& key : get_shared_keys(other)) {
				set(key, other.at(key));
			}
		}

		/**
		* @brief Reinitializes all elements in the map using a provided value generator function.
		* This overload is chosen for types that are copyable (have a copy constructor and copy assignment).
		* The capacity and index range remain unchanged.
		* @param value_generator A callable that takes a `KeyType const&` and
		* returns a `ValueType`. This is used to generate a new value for each slot.
		*
		* @note This method requires `ValueType` to be copy-assignable.
		*/
		void reinitialize_with_generator(fu2::function<ValueType(KeyType const&)> value_generator)
		requires std::copyable<ValueType> {
			for (iterator it = begin(); it < end(); it++) {
				auto& [key, value] = *it;
				value = value_generator(key); // Copy/Move assignment
			}
		}

		/**
		* @brief Reinitializes all elements in the map using a provided value generator function.
		* This overload is chosen for types that are movable but NOT copyable.
		* The capacity and index range remain unchanged.
		* @param value_generator A callable that takes a `KeyType const&` and
		* returns a `ValueType`. This is used to generate a new value for each slot.
		*
		* @note This method destroys existing elements and constructs new ones in place,
		* which is often more efficient for move-only types.
		*/
		void reinitialize_with_generator(fu2::function<ValueType(KeyType const&)> value_generator)
		requires std::movable<ValueType> && (!std::copyable<ValueType>) {
			values.clear();
			for (KeyType const& key : keys) {
				// Emplace directly, using move construction if generator returns rvalue
				values.emplace_back(value_generator(key));
			}
		}

		// --- Mathematical Operators (Valarray-like functionality) ---

		// Unary plus operator
		IndexedFlatMap<KeyType, ValueType> operator+() const {
			// Unary plus typically returns a copy of itself
			return *this;
		}

		// Unary minus operator
		IndexedFlatMap<KeyType, ValueType> operator-() const
		requires UnaryNegatable<ValueType> {
			static_assert(HasGetIndex<KeyType>);
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				return -this->at(key);
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap<KeyType, ValueType> operator+(IndexedFlatMap<KeyType,OtherValueType> const& other) const
		requires Addable<ValueType,OtherValueType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return IndexedFlatMap();
			}

			// Create a new map with the same keys as 'this'
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				if (other.contains(key)) {
					// If the key exists in 'other' (i.e., within its min_index/max_index),
					// perform the operation.
					return this->at(key) + other.at(key);
				} else {
					// If the key does not exist in 'other' (i.e., outside its min_index/max_index),
					// retain the value from 'this'.
					return this->at(key);
				}
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap<KeyType, ValueType> operator-(IndexedFlatMap<KeyType,OtherValueType> const& other) const
		requires Subtractable<ValueType,OtherValueType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return IndexedFlatMap();
			}

			return IndexedFlatMap(keys, [&](KeyType const& key) {
				if (other.contains(key)) {
					return this->at(key) - other.at(key);
				} else {
					return this->at(key);
				}
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap<KeyType, ValueType> operator*(IndexedFlatMap<KeyType,OtherValueType> const& other) const
		requires Multipliable<ValueType,OtherValueType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return IndexedFlatMap();
			}

			return IndexedFlatMap(keys, [&](KeyType const& key) {
				if (other.contains(key)) {
					return this->at(key) * other.at(key);
				} else {
					return this->at(key);
				}
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap<KeyType, ValueType> operator/(IndexedFlatMap<KeyType,OtherValueType> const& other) const
		requires Divisible<ValueType,OtherValueType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return IndexedFlatMap();
			}

			return IndexedFlatMap(keys, [&](KeyType const& key) {
				// Add a basic division by zero check for each element
				if (other.contains(key)) {
					if (other.at(key) == static_cast<ValueType>(0)) {
						spdlog::error_s(
							"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> division by zero detected at key index {}.",
							utility::type_name<KeyType>(),
							utility::type_name<ValueType>(),
							key.get_index()
						);
						assert(other.at(key) != static_cast<ValueType>(0));
						//continue and let it throw
					}
					return this->at(key) / other.at(key);
				} else {
					return this->at(key); // Retain original value if key not in 'other'
				}
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap<KeyType, ValueType> divide_handle_zero(
			IndexedFlatMap<KeyType,OtherValueType> const& other,
			fu2::function<ValueType(ValueType const&, OtherValueType const&)> handle_div_by_zero
		) const requires Divisible<ValueType,OtherValueType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return IndexedFlatMap();
			}

			return IndexedFlatMap(keys, [&](KeyType const& key) {
				if (other.contains(key)) {
					if (other.at(key) == static_cast<ValueType>(0)) {
						return handle_div_by_zero(
							this->at(key),
							other.at(key)
						);
					}
					return this->at(key) / other.at(key);
				} else {
					return this->at(key); // Retain original value if key not in 'other'
				}
			});
		}

		// Binary addition operator (Map + Scalar)
		template <typename ScalarType>
		IndexedFlatMap<KeyType, ValueType> operator+(ScalarType const& scalar) const
		requires Addable<ValueType,ScalarType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				return this->at(key) + scalar;
			});
		}

		// Binary subtraction operator (Map - Scalar)
		template <typename ScalarType>
		IndexedFlatMap<KeyType, ValueType> operator-(ScalarType const& scalar) const
		requires Subtractable<ValueType,ScalarType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				return this->at(key) - scalar;
			});
		}

		// Binary multiplication operator (Map * Scalar)
		template <typename ScalarType>
		IndexedFlatMap<KeyType, ValueType> operator*(ScalarType const& scalar) const
		requires Multipliable<ValueType,ScalarType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				return this->at(key) * scalar;
			});
		}

		// Binary division operator (Map / Scalar)
		template <typename ScalarType>
		IndexedFlatMap<KeyType, ValueType> operator/(ScalarType const& scalar) const
		requires Divisible<ValueType,ScalarType,ValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (scalar == static_cast<ValueType>(0)) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> division by zero for scalar operation.",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>()
				);
				assert(scalar != static_cast<ValueType>(0));
				//continue and let it throw
			}
			return IndexedFlatMap(keys, [&](KeyType const& key) {
				return this->at(key) / scalar;
			});
		}

		template <typename OtherValueType>
		IndexedFlatMap& operator+=(IndexedFlatMap<KeyType,OtherValueType> const& other)
		requires AddAssignable<ValueType,OtherValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return *this; // Return current state on error
			}

			// Iterate only over the keys that are present in 'other'
			for (KeyType const& key : other.get_keys()) {
				this->at(key) += other.at(key);
			}
			return *this;
		}

		template <typename OtherValueType>
		IndexedFlatMap& operator-=(IndexedFlatMap<KeyType,OtherValueType> const& other)
		requires SubtractAssignable<ValueType,OtherValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return *this;
			}

			for (KeyType const& key : other.get_keys()) {
				this->at(key) -= other.at(key);
			}
			return *this;
		}

		template <typename OtherValueType>
		IndexedFlatMap& operator*=(IndexedFlatMap<KeyType,OtherValueType> const& other)
		requires MultiplyAssignable<ValueType,OtherValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return *this;
			}

			for (KeyType const& key : other.get_keys()) {
				this->at(key) *= other.at(key);
			}
			return *this;
		}

		template <typename OtherValueType>
		IndexedFlatMap& operator/=(IndexedFlatMap<KeyType,OtherValueType> const& other)
		requires DivideAssignable<ValueType,OtherValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return *this;
			}

			for (KeyType const& key : other.get_keys()) {
				if (other.at(key) == static_cast<ValueType>(0)) {
					spdlog::error_s(
						"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> compound division by zero detected at key index {}.",
						utility::type_name<KeyType>(),
						utility::type_name<ValueType>(),
						key.get_index()
					);
					assert(other.at(key) != static_cast<ValueType>(0));
					//continue and let it throw
				}
				this->at(key) /= other.at(key);
			}
			return *this;
		}

		template <typename OtherValueType>
		IndexedFlatMap& divide_assign_handle_zero(
			IndexedFlatMap<KeyType,OtherValueType> const& other,
			fu2::function<void(ValueType&, OtherValueType const&)> handle_div_by_zero
		) requires DivideAssignable<ValueType,OtherValueType> {
			static_assert(HasGetIndex<KeyType>);
			if (!check_subset_span_match(other)) {
				return *this;
			}

			for (KeyType const& key : other.get_keys()) {
				if (other.at(key) == static_cast<ValueType>(0)) {
					handle_div_by_zero(
						this->at(key),
						other.at(key)
					);
				} else {
					this->at(key) /= other.at(key);
				}
			}
			return *this;
		}

		// Compound assignment addition operator (Map += Scalar)
		template <typename ScalarType>
		IndexedFlatMap& operator+=(ScalarType const& scalar)
		requires AddAssignable<ValueType,ScalarType> {
			for (ValueType& val : values) {
				val += scalar;
			}
			return *this;
		}

		// Compound assignment subtraction operator (Map -= Scalar)
		template <typename ScalarType>
		IndexedFlatMap& operator-=(ScalarType const& scalar)
		requires SubtractAssignable<ValueType,ScalarType> {
			for (ValueType& val : values) {
				val -= scalar;
			}
			return *this;
		}

		// Compound assignment multiplication operator (Map *= Scalar)
		template <typename ScalarType>
		IndexedFlatMap& operator*=(ScalarType const& scalar)
		requires MultiplyAssignable<ValueType,ScalarType> {
			for (ValueType& val : values) {
				val *= scalar;
			}
			return *this;
		}

		// Compound assignment division operator (Map /= Scalar)
		template <typename ScalarType>
		IndexedFlatMap& operator/=(ScalarType const& scalar)
		requires DivideAssignable<ValueType,ScalarType> {
			if (scalar == static_cast<ValueType>(0)) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> compound division by zero for scalar operation.",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>()
				);
				assert(scalar != static_cast<ValueType>(0));
				//continue and let it throw
			}
			for (ValueType& val : values) {
				val /= scalar;
			}
			return *this;
		}

		template <typename OtherValueType,typename ScalarType>
		constexpr IndexedFlatMap& mul_add(
			IndexedFlatMap<KeyType,OtherValueType> const& other,
			ScalarType const& factor
		) requires MulAddAssignable<ValueType,OtherValueType,ScalarType> {
			static_assert(HasGetIndex<KeyType>);
			for (KeyType const& key : get_shared_keys(other)) {
				at(key) += other.at(key) * factor;
			}

			return *this;
		}

		template <typename ValueTypeA,typename ValueTypeB>
		constexpr IndexedFlatMap& mul_add(
			IndexedFlatMap<KeyType,ValueTypeA> const& a,
			IndexedFlatMap<KeyType,ValueTypeB> const& b
		) requires MulAddAssignable<ValueType,ValueTypeA,ValueTypeB> {
			static_assert(HasGetIndex<KeyType>);
			if (a.get_min_index() != b.get_min_index() || a.get_max_index() != b.get_max_index()) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> attempted mul_add where a and b don't have the same keys. This is not implemented.",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>()
				);
				assert(a.get_min_index() == b.get_min_index() && a.get_max_index() == b.get_max_index());
			}

			for (KeyType const& key : get_shared_keys(a)) {
				at(key) += a.at(key) * b.at(key);
			}

			return *this;
		}

		template <typename ScalarType>
		constexpr IndexedFlatMap& rescale(ScalarType const& new_total)
		requires std::default_initializable<ValueType>
			&& AddAssignable<ValueType>
			&& MultiplyAssignable<ValueType,ScalarType>
			&& DivideAssignable<ValueType> {
			static_assert(HasGetIndex<KeyType>);
			bool has_any_non_zero_value = false;
			const ValueType zero = static_cast<ValueType>(0);
			ValueType old_total {};
			for (ValueType const& value : values) {
				old_total += value;
				has_any_non_zero_value |= value != zero;
			}
			IndexedFlatMap& this_ref = *this;
			if (has_any_non_zero_value) {
				this_ref *= new_total;
				this_ref /= old_total;
			}
			return this_ref;
		}

		// --- Iterators for Key-Value Pairs ---
		template <bool IsConst>
		class BasicIterator {
		public:
			using value_type = std::pair<KeyType const&, std::conditional_t<IsConst, ValueType const&, ValueType&>>;
			using difference_type = std::ptrdiff_t;
			using reference = value_type;
			using iterator_category = std::random_access_iterator_tag;

		private:
			std::conditional_t<IsConst, typename values_vector_type::const_iterator, typename values_vector_type::iterator> value_it;
			typename keys_span_type::iterator key_it;

		public:
			// Constructor
			BasicIterator(
				std::conditional_t<IsConst, typename values_vector_type::const_iterator, typename values_vector_type::iterator> val_it,
				typename keys_span_type::iterator k_it
			) : value_it(val_it), key_it(k_it) {}

			// Dereference operator
			reference operator*() const {
				return {*key_it, *value_it};
			}

			// Arrow operator (for member access)
			// Note: This returns a proxy object, as std::pair is not a class type.
			// For direct member access, it's often better to dereference and use dot operator: (*it).first
			// However, for compatibility with some algorithms, a proxy is provided.
			struct Proxy {
				KeyType const& first;
				std::conditional_t<IsConst, ValueType const&, ValueType&> second;
				// Add operator-> for nested access if needed, e.g., Proxy->first.some_member()
				// For now, direct access to first/second is assumed.
			};
			Proxy operator->() const {
				return {*key_it, *value_it};
			}

			// Pre-increment
			BasicIterator& operator++() {
				++value_it;
				++key_it;
				return *this;
			}

			// Post-increment
			BasicIterator operator++(int) {
				BasicIterator temp = *this;
				++(*this);
				return temp;
			}

			// Pre-decrement
			BasicIterator& operator--() {
				--value_it;
				--key_it;
				return *this;
			}

			// Post-decrement
			BasicIterator operator--(int) {
				BasicIterator temp = *this;
				--(*this);
				return temp;
			}

			// Random access operators
			BasicIterator& operator+=(difference_type n) {
				value_it += n;
				key_it += n;
				return *this;
			}

			BasicIterator operator+(difference_type n) const {
				BasicIterator temp = *this;
				temp += n;
				return temp;
			}

			BasicIterator& operator-=(difference_type n) {
				value_it -= n;
				key_it -= n;
				return *this;
			}

			BasicIterator operator-(difference_type n) const {
				BasicIterator temp = *this;
				temp -= n;
				return temp;
			}

			difference_type operator-(BasicIterator const& other) const {
				return value_it - other.value_it;
			}

			// Comparison operators
			bool operator==(BasicIterator const& other) const {
				return value_it == other.value_it;
			}

			bool operator!=(BasicIterator const& other) const {
				return !(*this == other);
			}

			bool operator<(BasicIterator const& other) const {
				return value_it < other.value_it;
			}

			bool operator>(BasicIterator const& other) const {
				return value_it > other.value_it;
			}

			bool operator<=(BasicIterator const& other) const {
				return value_it <= other.value_it;
			}

			bool operator>=(BasicIterator const& other) const {
				return value_it >= other.value_it;
			}

			// Conversion to const_iterator
			operator BasicIterator<true>() const {
				return BasicIterator<true>(value_it, key_it);
			}
		};

		using iterator = BasicIterator<false>;
		using const_iterator = BasicIterator<true>;

		// Begin and End methods
		// note cbegin & cend for std::span<T> requires c++23 so begin & end are used instead
		iterator begin() {
			return iterator(values.begin(), keys.begin());
		}

		const_iterator begin() const {
			return const_iterator(values.cbegin(), keys.begin());
		}

		const_iterator cbegin() const {
			return const_iterator(values.cbegin(), keys.begin());
		}

		iterator end() {
			return iterator(values.end(), keys.end());
		}

		const_iterator end() const {
			return const_iterator(values.cend(), keys.end());
		}

		const_iterator cend() const {
			return const_iterator(values.cend(), keys.end());
		}
	};

	// Non-member binary operators for (Scalar op Map)
	template <typename ScalarType, typename KeyType, typename ValueType>
	IndexedFlatMap<KeyType, ValueType> operator+(
		ScalarType const& scalar,
		IndexedFlatMap<KeyType, ValueType> const& map
	) requires Addable<ValueType,ScalarType,ValueType> {
		return map + scalar; // Delegate to the member operator
	}

	template <typename ScalarType, typename KeyType, typename ValueType>
	IndexedFlatMap<KeyType, ValueType> operator-(
		ValueType const& scalar,
		IndexedFlatMap<KeyType, ValueType> const& map
	) requires Subtractable<ScalarType,ValueType,ValueType> {
		static_assert(HasGetIndex<KeyType>);
		// Scalar - Map is not simply map - scalar, so we implement it directly
		return IndexedFlatMap<KeyType, ValueType>(map.get_keys(), [&](KeyType const& key) {
			return scalar - map.at(key);
		});
	}

	template <typename ScalarType, typename KeyType, typename ValueType>
	IndexedFlatMap<KeyType, ValueType> operator*(
		ScalarType const& scalar,
		IndexedFlatMap<KeyType, ValueType> const& map
	) requires Multipliable<ValueType,ScalarType,ValueType> {
		return map * scalar; // Delegate to the member operator
	}

	template <typename ScalarType, typename KeyType, typename ValueType>
	IndexedFlatMap<KeyType, ValueType> operator/(
		ScalarType const& scalar,
		IndexedFlatMap<KeyType, ValueType> const& map
	) requires Divisible<ScalarType, ValueType, ValueType> {
		static_assert(HasGetIndex<KeyType>);
		return IndexedFlatMap<KeyType, ValueType>(map.get_keys(), [&](KeyType const& key) {
			if (map.at(key) == static_cast<ValueType>(0)) {
				spdlog::error_s(
					"DEVELOPER: OpenVic::IndexedFlatMap<{},{}> scalar division by zero detected at key index {}.",
					utility::type_name<KeyType>(),
					utility::type_name<ValueType>(),
					key.get_index()
				);
				assert(map.at(key) != static_cast<ValueType>(0));
				//continue and let it throw
			}
			return scalar / map.at(key);
		});
	}
}