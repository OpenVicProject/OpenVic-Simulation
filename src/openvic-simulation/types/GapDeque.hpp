#pragma once

// Based heavily on
// https://github.com/slavenf/sfl-library/blob/2c6f464f5101caa39640d765002907e29b5b3acd/include/sfl/segmented_vector.hpp
// modified to suit OpenVic's purposes
//
// Copyright (c) 2022 Slaven Falandys
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>

#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	namespace detail {
		template<typename T, typename Value>
		concept GapDequeAddValue = utility::any_of<T, const Value&, Value&&>;
		template<typename T, typename Iterator>
		concept GapDequeEraseIt = utility::any_of<T, Iterator, const Iterator>;
	}

	template<typename T, typename Value>
	concept GapDequeStorage = requires(const T& ct, T& t, const Value& ref_val, Value&& rval) {
		{ ct.has_value() } -> std::same_as<bool>;
		{ t.value() } -> std::same_as<Value&>;
		{ ct.value() } -> std::same_as<const Value&>;
		{ t.emplace(ref_val) } -> std::same_as<Value&>;
		{ t.emplace(std::move(rval)) } -> std::same_as<Value&>;
		{ t.reset() } -> std::same_as<void>;
		{ T() } -> std::same_as<T>;
		{ T(std::move(rval)) } -> std::same_as<T>;
	};

	template<
		typename ValueType,
		GapDequeStorage<ValueType> StorageType, // std::iterator_traits<Iter>::value_type
		typename Pointer, // std::iterator_traits<Iter>::pointer
		typename Reference, // std::iterator_traits<Iter>::reference
		typename DifferenceType, // std::iterator_traits<Iter>::difference_type
		typename BlockPointer, // Non-const pointer to block.
		typename ElementPointer, // Non-const pointer to element.
		std::size_t BlockSize>
	class GapDequeIterator {
		template<typename, typename, typename, typename, typename, typename, typename, std::size_t>
		friend class GapDequeIterator;

		template<typename, std::size_t, typename, typename>
		friend class GapDeque;

	public:
		using value_type = StorageType;
		using pointer = Pointer;
		using reference = Reference;
		using difference_type = DifferenceType;
		using iterator_category = std::bidirectional_iterator_tag;

		static constexpr std::integral_constant<std::size_t, BlockSize> block_size {};

	private:
		using block_pointer = BlockPointer;
		using element_pointer = ElementPointer;

		block_pointer _block;
		element_pointer _element;

	public:
		GapDequeIterator() {}
		GapDequeIterator(const GapDequeIterator& other) noexcept : _block(other._block), _element(other._element) {}
		template<std::convertible_to<Pointer> OtherPointer, typename OtherReference>
		GapDequeIterator(const GapDequeIterator<
						 ValueType, StorageType, OtherPointer, OtherReference, DifferenceType, BlockPointer, ElementPointer,
						 BlockSize>& other) noexcept
			: _block(other._block), _element(other._element) {}

		GapDequeIterator& operator=(const GapDequeIterator& other) noexcept {
			_block = other._block;
			_element = other._element;
			return *this;
		}

		[[nodiscard]] reference operator*() {
			return (*_element).value();
		}

		[[nodiscard]] pointer operator->() {
			return &_element.value();
		}

		GapDequeIterator& operator++() {
			increment_once();
			return *this;
		}

		GapDequeIterator operator++(int) {
			auto temp = *this;
			increment_once();
			return temp;
		}

		GapDequeIterator& operator--() {
			decrement_once();
			return *this;
		}

		GapDequeIterator operator--(int) {
			auto temp = *this;
			decrement_once();
			return temp;
		}

		// GapDequeIterator& operator+=(difference_type n) {
		// 	advance(n);
		// 	return *this;
		// }

		// GapDequeIterator& operator-=(difference_type n) {
		// 	advance(-n);
		// 	return *this;
		// }

		// [[nodiscard]] GapDequeIterator operator+(difference_type n) const {
		// 	auto temp = *this;
		// 	temp.advance(n);
		// 	return temp;
		// }

		// [[nodiscard]] GapDequeIterator operator-(difference_type n) const {
		// 	auto temp = *this;
		// 	temp.advance(-n);
		// 	return temp;
		// }

		// [[nodiscard]] reference operator[](difference_type n) const {
		// 	auto temp = *this;
		// 	temp.advance(n);
		// 	return *temp;
		// }

		[[nodiscard]] friend GapDequeIterator operator+(difference_type n, const GapDequeIterator& it) {
			return it + n;
		}

		[[nodiscard]] friend difference_type operator-(const GapDequeIterator& x, const GapDequeIterator& y) {
			return (x._block - y._block) * difference_type(block_size()) + (x._element - *x._block) - (y._element - *y._block);
		}

		[[nodiscard]] friend bool operator==(const GapDequeIterator& x, const GapDequeIterator& y) {
			return x._element == y._element;
		}

		[[nodiscard]] friend bool operator!=(const GapDequeIterator& x, const GapDequeIterator& y) {
			return !(x == y);
		}

		[[nodiscard]] friend bool operator<(const GapDequeIterator& x, const GapDequeIterator& y) {
			return (x._block == y._block) ? (x._element < y._element) : (x._block < y._block);
		}

		[[nodiscard]] friend bool operator>(const GapDequeIterator& x, const GapDequeIterator& y) {
			return y < x;
		}

		[[nodiscard]] friend bool operator<=(const GapDequeIterator& x, const GapDequeIterator& y) {
			return !(y < x);
		}

		[[nodiscard]] friend bool operator>=(const GapDequeIterator& x, const GapDequeIterator& y) {
			return !(x < y);
		}

	private:
		GapDequeIterator(block_pointer block, element_pointer elem) noexcept : _block(block), _element(elem) {}

		void increment_once() noexcept {
			do {
				++_element;
				if (_element == *_block + block_size()) {
					++_block;
					_element = *_block;
				}
			} while (_element && !_element->has_value());
		}

		void decrement_once() noexcept {
			do {
				if (_element == *_block) {
					--_block;
					_element = *_block + block_size();
				}
				--_element;
			} while (_element && !_element->has_value());
		}

		// Ignores has_value check
		void advance(difference_type n) noexcept {
			const difference_type offset = n + (_element - *_block);

			if (offset >= 0 && offset < difference_type(block_size())) {
				_element += n;
			} else {
				const difference_type _block_offset =
					offset > 0 ? offset / difference_type(block_size()) : -difference_type((-offset - 1) / block_size()) - 1;

				_block += _block_offset;

				_element = *_block + (offset - _block_offset * difference_type(block_size()));
			}
		}
	};

	template<
		typename T, std::size_t BlockSize, GapDequeStorage<T> Storage = std::optional<T>,
		class Allocator = std::allocator<Storage>>
	class GapDeque {
		static_assert(BlockSize > 0, _OV_STR(BlockSize) " must be greater than zero.");

	public:
		struct Iterator;
		using value_type = T;
		using storage_type = Storage;
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;
		using size_type = typename allocator_traits::size_type;
		using difference_type = typename allocator_traits::difference_type;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using storage_pointer = typename allocator_traits::pointer;
		using const_storage_pointer = typename allocator_traits::const_pointer;

		static constexpr std::integral_constant<std::size_t, BlockSize> block_size {};

	private:
		using block_allocator = typename allocator_traits::template rebind_alloc<pointer>;
		using block_pointer = typename allocator_traits::pointer;

	public:
		using iterator = GapDequeIterator<
			value_type, storage_type, storage_pointer, reference, difference_type, block_pointer, pointer, block_size>;
		using const_iterator = GapDequeIterator<
			value_type, storage_type, const_storage_pointer, const_reference, difference_type, block_pointer, pointer,
			block_size>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static_assert(
			std::is_same<typename Allocator::value_type, storage_type>::value,
			_OV_STR(Allocator::value_type) " must be same as " _OV_STR(OpenVic::GapDeque::storage_type) "."
		);

		GapDeque() {}

		[[nodiscard]] allocator_type get_allocator() const {
			return _data.ref_to_alloc();
		}

		[[nodiscard]] reference front() {
			return *_data._front;
		}
		[[nodiscard]] reference back() {
			auto temp = _data._last;
			--temp;
			return *temp;
		}

		[[nodiscard]] const_reference front() const {
			return *_data._front;
		}
		[[nodiscard]] const_reference back() const {
			auto temp = _data._last;
			--temp;
			return *temp;
		}

		[[nodiscard]] iterator begin() {
			return _data._first;
		}
		[[nodiscard]] iterator end() {
			return _data._last;
		}

		[[nodiscard]] const_iterator begin() const {
			return _data._first;
		}
		[[nodiscard]] const_iterator end() const {
			return _data._last;
		}

		[[nodiscard]] const_iterator cbegin() const {
			return _data._first;
		}
		[[nodiscard]] const_iterator cend() const {
			return _data._last;
		}

		[[nodiscard]] reverse_iterator rbegin() {
			return reverse_iterator(end());
		}
		[[nodiscard]] reverse_iterator rend() {
			return reverse_iterator(begin());
		}

		[[nodiscard]] const_reverse_iterator rbegin() const {
			return const_reverse_iterator(end());
		}
		[[nodiscard]] const_reverse_iterator rend() const {
			return const_reverse_iterator(begin());
		}

		[[nodiscard]] const_reverse_iterator crbegin() const {
			return const_reverse_iterator(end());
		}
		[[nodiscard]] const_reverse_iterator crend() const {
			return const_reverse_iterator(begin());
		}

		[[nodiscard]] bool empty() const {
			return _data._last == _data._first;
		}
		[[nodiscard]] size_type size() const {
			return _data._last - _data._first;
		}
		[[nodiscard]] size_type capacity() const {
			return _data._end - _data._first;
		}
		[[nodiscard]] size_type max_size() const {
			return std::min<size_type>(
				allocator_traits::max_size(_data.ref_to_alloc()),
				std::numeric_limits<difference_type>::max() / sizeof(value_type)
			);
		}

		void clear() {
			erase(begin(), end());
		}

		void clear_and_shrink() {
			destroy(_data.ref_to_alloc(), _data._first, _data._last);
			_data._last = _data._first;
			_data._first_invalid = begin();
		}

	private:
		iterator set(const_iterator pos, detail::GapDequeAddValue<value_type> auto value) {
			iterator p(pos._block, pos._element);
			if (_data._first_invalid > p) {
				while (_data._first_invalid._element->has_value()) {
					_data._first_invalid.advance(1);
				}
			}
			if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
				return p._element->emplace(std::move(value));
			} else {
				return p._element->emplace(value);
			}
		}

		/*iterator insert(const_iterator pos, detail::GapDequeAddValue<value_type> auto value) {
			if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
				if (!pos._element->has_value()) {
					return set(pos, std::move(value));
				}
				return emplace(pos, std::move(value));
			} else {
				if (!pos._element->has_value()) {
					return set(pos, value);
				}
				return emplace(pos, value);
			}
		}*/

		template<class... Args>
		iterator emplace(const_iterator pos, Args&&... args) {
			iterator p(pos._block, pos._element);

			if (_data._last == _data._end) {
				const size_type offset = pos - cbegin();

				grow_storage(1);

				p = nth(offset);
			}

			if (p == _data._last) {
				GapDeque::construct_at(_data.ref_to_alloc(), _data._last._element, std::forward<Args>(args)...);

				++_data._last;
			} else {
				// The order of operations is critical. First we will construct
				// temporary value because arguments `args...` can contain
				// reference to element in this container and after that
				// we will move elements and insert new element.

				temporary_value temp(_data.ref_to_alloc(), std::forward<Args>(args)...);

				GapDeque::construct_at(_data.ref_to_alloc(), _data._last._element, std::move(*(_data._last - 1)));

				++_data._last;

				std::move_backward(p, _data._last - 2, _data._last - 1);

				*p = std::move(temp.value());
			}

			return p;
		}

	public:
		iterator erase(detail::GapDequeEraseIt<iterator> auto pos) {
			assert(cbegin() <= pos && pos < cend());
			iterator p(pos._block, pos._element);
			if (p < _data._first_invalid) {
				_data._first_invalid = p;
			}
			p._element->reset();
			p--;
			return p;
		}
		template<detail::GapDequeEraseIt<iterator> it>
		iterator erase(it first, it last) {
			assert(cbegin() <= first && first <= last && last <= cend());
			iterator p(first._block, first._element);
			if (first == last) {
				return p;
			}

			if (p < _data._first_invalid) {
				_data._first_invalid = p;
			}

			while (p <= last) {
				p._element->reset();
				p++;
			}
			p--;
			return p;
		}

		void shrink_erased() {
			iterator p = begin();
			size_type empty_count = 0;
			while (p != end()) {
				if (!p._element->has_value()) {
					empty_count++;
				} else {
					const difference_type dist = std::distance(p - empty_count, p);
					if (p + dist < _data._last) {
						std::move(p + dist, _data._last, p);
					}

					iterator new_last = _data._last - dist;

					GapDeque::destroy(_data.ref_to_alloc(), new_last, _data._last);

					_data._last = new_last;
				}
				p = p.advance(1);
			}

			_data._first_invalid = end();
		}

		template<typename... Args>
		reference emplace_back(Args&&... args) {
			return *emplace(cend(), std::forward<Args>(args)...);
		}

		void pop_back() {
			assert(!empty());
			iterator p = end();
			p--;
			if (_data._first_invalid > p) {
				_data._first_invalid = p;
			}
			if (p) {
				p._element->reset();
			}
		}

		void push_back(detail::GapDequeAddValue<value_type> auto value) {
			if constexpr (std::is_rvalue_reference_v<decltype(value)>) {
				if (_data._first_invalid != end()) {
					set(_data._first_invalid, std::move(value));
					return;
				}
				emplace(cend(), std::move(value));
			} else {
				if (_data._first_invalid != end()) {
					set(_data._first_invalid, value);
					return;
				}
				emplace(cend(), value);
			}
		}

		void reserve(size_type new_cap) {
			const size_type _capacity = capacity();
			if (new_cap > _capacity) {
				grow_storage(new_cap - _capacity);
			}
		}

		// void resize(size_type n) {
		// 	const size_type s = size();

		// 	if (n > s) {
		// 		const size_type delta = n - s;

		// 		const size_type c = capacity();

		// 		if (n > c) {
		// 			grow_storage(n - c);
		// 		}

		// 		_data._last = GapDeque::uninitialized_default_construct_n(_data.ref_to_alloc(), _data._last, delta);
		// 	} else if (n < s) {
		// 		iterator new_last = nth(n);

		// 		GapDeque::destroy(_data.ref_to_alloc(), new_last, _data._last);

		// 		_data._last = new_last;
		// 	}
		// }

		void shrink_to_fit() {
			{
				auto new_block_last = _data._last._block + 1;

				destroy_segments(new_block_last, _data._block_last);

				_data._block_last = new_block_last;

				_data._end._block = _data._block_last - 1;
				_data._end._element = *(_data._block_last - 1) + (block_size() - 1);
			}

			// Shrink table.
			{
				const size_type table_capacity = _data._block_end - _data._block_first;

				const size_type table_size = _data._block_last - _data._block_first;

				const size_type new_table_capacity = std::max(min_table_capacity(), table_size);

				const size_type n = _data._last._block - _data._first._block;
				const size_type m = _data._end._block - _data._first._block;

				// Allocate new table. No effects if allocation fails.
				block_pointer new_block_first = allocate_table(new_table_capacity);
				block_pointer new_block_last = new_block_first;
				block_pointer new_block_end = new_block_first + new_table_capacity;

				// Copy pointers (noexcept).
				new_block_last = std::copy(_data._block_first, _data._block_last, new_block_first);

				// Deallocate old table (noexcept).
				deallocate_table(_data._block_first, table_capacity);

				// Update pointers (noexcept).
				_data._block_first = new_block_first;
				_data._block_last = new_block_last;
				_data._block_end = new_block_end;

				// Update iterators (noexcept).
				_data._first._block = new_block_first;
				_data._last._block = new_block_first + n;
				_data._end._block = new_block_first + m;
			}
		}

		void swap(GapDeque& other) noexcept {
			assert(allocator_traits::propagate_on_container_swap::value || _data.ref_to_alloc() == other._data.ref_to_alloc());

			if (this == &other) {
				return;
			}

			using std::swap;

			if (allocator_traits::propagate_on_container_swap::value) {
				swap(_data.ref_to_alloc(), other._data.ref_to_alloc());
			}

			swap(_data, other._data);
		}

	private:
		struct BlockData {
			block_pointer _block_first;
			block_pointer _block_last;
			block_pointer _block_end;

			iterator _first;
			iterator _last;
			iterator _end;

			iterator _first_invalid;
		};

		struct Data : BlockData, allocator_type {
			Data() noexcept(std::is_nothrow_default_constructible<allocator_type>::value) : allocator_type() {}

			Data(const allocator_type& alloc) noexcept(std::is_nothrow_copy_constructible<allocator_type>::value)
				: allocator_type(alloc) {}

			Data(allocator_type&& other) noexcept(std::is_nothrow_move_constructible<allocator_type>::value)
				: allocator_type(std::move(other)) {}

			allocator_type& ref_to_alloc() noexcept {
				return *this;
			}

			const allocator_type& ref_to_alloc() const noexcept {
				return *this;
			}
		} _data;

		[[nodiscard]] reference operator[](size_type index) {
			assert(index < size());

			const auto i = index / block_size();
			const auto j = index - i * block_size();

			return *(*(_data._block_first + i) + j);
		}
		[[nodiscard]] const_reference operator[](size_type index) const {
			assert(index < size());

			const auto i = index / block_size();
			const auto j = index - i * block_size();

			return *(*(_data._block_first + i) + j);
		}

		[[nodiscard]] iterator nth(size_type pos) noexcept {
			assert(pos <= size());

			const auto i = pos / block_size();
			const auto j = pos - i * block_size();

			const auto block = _data._block_first + i;
			const auto element = *block + j;

			return iterator(block, element);
		}

		[[nodiscard]] const_iterator nth(size_type pos) const noexcept {
			assert(pos <= size());

			const auto i = pos / block_size();
			const auto j = pos - i * block_size();

			const auto block = _data._block_first + i;
			const auto element = *block + j;

			return iterator(block, element);
		}

		template<typename AddrT>
		static constexpr AddrT* to_address(AddrT* p) noexcept {
			static_assert(!std::is_function<AddrT>::value, "not a function pointer");
			return p;
		}

		template<typename Pointer>
		static constexpr auto to_address(const Pointer& p) noexcept -> typename std::pointer_traits<Pointer>::element_type* {
			return GapDeque::to_address(p.operator->());
		}

		template<typename Alloc, typename Size>
		static auto allocate(Alloc& a, Size n) -> typename std::allocator_traits<Alloc>::pointer {
			if (n != 0) {
				return std::allocator_traits<Alloc>::allocate(a, n);
			}
			return nullptr;
		}

		template<typename Alloc, typename Pointer, typename Size>
		static void deallocate(Alloc& a, Pointer p, Size n) noexcept {
			if (p != nullptr) {
				std::allocator_traits<Alloc>::deallocate(a, p, n);
			}
		}

		template<typename Alloc, typename Pointer, typename... Args>
		static void construct_at(Alloc& a, Pointer p, Args&&... args) {
			std::allocator_traits<Alloc>::construct(a, GapDeque::to_address(p), std::forward<Args>(args)...);
		}

		template<typename Alloc, typename Pointer>
		static void destroy_at(Alloc& a, Pointer p) noexcept {
			std::allocator_traits<Alloc>::destroy(a, GapDeque::to_address(p));
		}

		template<typename Alloc, typename ForwardIt>
		static void destroy(Alloc& a, ForwardIt first, ForwardIt last) noexcept {
			while (first != last) {
				GapDeque::destroy_at(a, std::addressof(*first));
				++first;
			}
		}

		template<typename Alloc, typename ForwardIt, typename Size>
		static void destroy_n(Alloc& a, ForwardIt first, Size n) noexcept {
			while (n > 0) {
				GapDeque::destroy_at(a, std::addressof(*first));
				++first;
				--n;
			}
		}

		template<typename Alloc, typename ForwardIt, typename Size>
		static ForwardIt uninitialized_default_construct_n(Alloc& a, ForwardIt first, Size n) {
			ForwardIt curr = first;
			if (true) {
				while (n > 0) {
					GapDeque::construct_at(a, std::addressof(*curr));
					++curr;
					--n;
				}
				return curr;
			}
			if (false) {
				GapDeque::destroy(a, first, curr);
				// SFL_RETHROW;
			}
		}

		block_pointer allocate_table(size_type n) {
			block_allocator block_alloc(_data.ref_to_alloc());
			return GapDeque::allocate(block_alloc, n);
		}

		/// Deallocates table at `p`.
		/// Does not deallocates segments, it only deallocates memory used by table.
		///
		void deallocate_table(block_pointer p, size_type n) noexcept {
			block_allocator block_alloc(_data.ref_to_alloc());
			GapDeque::deallocate(block_alloc, p, n);
		}

		/// Allocates a segment and returns pointer to it.
		/// Does not construct elements, it only allocates memory for segment.
		///
		pointer allocate_segment() {
			return GapDeque::allocate(_data.ref_to_alloc(), block_size());
		}

		/// Deallocates segments at `p`.
		/// Does not destroy elements, it only deallocates memory used by segment.
		///
		void deallocate_segment(pointer p) noexcept {
			GapDeque::deallocate(_data.ref_to_alloc(), p, block_size());
		}

		/// Allocates segments in table in range [first, last).
		/// Does not construct elements, it only allocates memory for segments.
		///
		void construct_segments(block_pointer first, block_pointer last) {
			block_pointer curr = first;

			if (true) {
				while (curr != last) {
					*curr = allocate_segment();
					++curr;
				}
			}
			if (false) {
				destroy_segments(first, curr);
				// SFL_RETHROW;
			}
		}

		/// Destroys segments in table in range [first, last).
		/// Does not destroy elements, it only deallocates memory used by segments.
		///
		void destroy_segments(block_pointer first, block_pointer last) noexcept {
			while (first != last) {
				deallocate_segment(*first);
				++first;
			}
		}

		static constexpr size_type min_table_capacity() noexcept {
			return 8;
		}

		static constexpr double table_grow_factor() noexcept {
			return 1.5;
		}

		void construct_storage(size_type num_elements) {
			if (num_elements > max_size()) {
				std::abort();
				// SFL_DTL::throw_length_error("sfl::segmented_vector::construct_storage");
			}

			const size_type num_segments = num_elements / block_size() + 1;

			const size_type table_capacity = std::max(min_table_capacity(), num_segments);

			_data._block_first = allocate_table(table_capacity);
			_data._block_last = _data._block_first + num_segments;
			_data._block_end = _data._block_first + table_capacity;

			if (true) {
				construct_segments(_data._block_first, _data._block_last);

				_data._first._block = _data._block_first;
				_data._first._element = *_data._block_first;

				_data._last = _data._first;

				_data._end._block = _data._block_last - 1;
				_data._end._element = *(_data._block_last - 1) + (block_size() - 1);
			}
			if (false) {
				deallocate_table(_data._block_first, table_capacity);
				// SFL_RETHROW;
			}
		}

		void grow_storage(size_type num_additional_elements) {
			if (max_size() - capacity() < num_additional_elements) {
				std::abort();
				// SFL_DTL::throw_length_error("sfl::segmented_vector::grow_storage");
			}

			const size_type num_additional_segments = num_additional_elements / block_size() + 1;

			const size_type available_table_capacity = _data._block_end - _data._block_last;

			// Grow table if neccessary.
			if (num_additional_segments > available_table_capacity) {
				const size_type table_capacity = _data._block_end - _data._block_first;

				const size_type table_size = _data._block_last - _data._block_first;

				const size_type new_table_capacity =
					std::max<size_type>(table_capacity * table_grow_factor(), table_size + num_additional_segments);

				const size_type n = _data._last._block - _data._first._block;
				const size_type m = _data._end._block - _data._first._block;

				// Allocate new table. No effects if allocation fails.
				block_pointer new_block_first = allocate_table(new_table_capacity);
				block_pointer new_block_last = new_block_first;
				block_pointer new_block_end = new_block_first + new_table_capacity;

				// Copy pointers (noexcept).
				new_block_last = std::copy(_data._block_first, _data._block_last, new_block_first);

				// Deallocate old table (noexcept).
				deallocate_table(_data._block_first, table_capacity);

				// Update pointers (noexcept).
				_data._block_first = new_block_first;
				_data._block_last = new_block_last;
				_data._block_end = new_block_end;

				// Update iterators (noexcept).
				_data._first._block = new_block_first;
				_data._last._block = new_block_first + n;
				_data._end._block = new_block_first + m;
			}

			auto new_block_last = _data._block_last + num_additional_segments;

			// No effects in allocation fails.
			construct_segments(_data._block_last, new_block_last);

			_data._block_last = new_block_last;

			_data._end._block = _data._block_last - 1;
			_data._end._element = *(_data._block_last - 1) + (block_size() - 1);
		}

		void destroy_storage() noexcept {
			destroy_segments(_data._block_first, _data._block_last);
			deallocate_table(_data._block_first, _data._block_end - _data._block_first);
		}

		void initialize_empty() {
			construct_storage(0);
		}

		class temporary_value {
		private:
			allocator_type& _alloc;

			alignas(storage_type) unsigned char _buffer[sizeof(storage_type)];

			value_type* buffer() {
				return reinterpret_cast<storage_type*>(_buffer)->value();
			}

		public:
			template<typename... Args>
			explicit temporary_value(allocator_type& a, Args&&... args) : _alloc(a) {
				GapDeque::construct_at(_alloc, buffer(), std::forward<Args>(args)...);
			}

			~temporary_value() {
				GapDeque::destroy_at(_alloc, buffer());
			}

			storage_type& value() {
				return *buffer();
			}
		};
	};
}
