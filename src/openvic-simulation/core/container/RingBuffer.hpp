#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include <range/v3/algorithm/move.hpp>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/view/subrange.hpp>

#include "openvic-simulation/core/Assert.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

namespace OpenVic {

	/// RingBuffer datatype
	/// Generally most of the interface should be a drop-in replacement for std::vector
	template<typename T, typename Allocator = std::allocator<T>>
	struct RingBuffer {
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;
		using value_type = T;
		using size_type = typename allocator_traits::size_type;
		using difference_type = typename allocator_traits::difference_type;
		using pointer = typename allocator_traits::pointer;
		using const_pointer = typename allocator_traits::const_pointer;
		using reference = decltype(*pointer {});
		using const_reference = decltype(*const_pointer {});

		template<typename PointerType>
		struct _iterator {
			using difference_type = typename allocator_traits::difference_type;
			using size_type = typename allocator_traits::difference_type;
			using value_type = typename allocator_traits::value_type;
			using pointer = PointerType;
			using reference = decltype(*pointer {});
			using iterator_category = std::random_access_iterator_tag;

			constexpr _iterator() noexcept = default;
			_iterator(pointer data, const size_type ring_offset, const size_type ring_index, const size_type ring_capacity)
				: _data(data), _offset(ring_offset), _index(ring_index), _capacity(ring_capacity) {}

			constexpr operator _iterator<typename allocator_traits::const_pointer>() const {
				return _iterator<typename allocator_traits::const_pointer>(_data, _offset, _index, _capacity);
			}

			constexpr reference operator*() const {
				return _data[_ring_wrap(_offset + _index, _capacity)];
			}

			constexpr pointer operator->() const noexcept {
				return &**this;
			}

			_iterator operator++(int) noexcept {
				_iterator copy(*this);
				operator++();
				return copy;
			}

			_iterator& operator++() noexcept {
				++_index;
				return *this;
			}

			_iterator operator--(int) noexcept {
				_iterator copy(*this);
				operator--();
				return copy;
			}

			_iterator& operator--() noexcept {
				--_index;
				return *this;
			}

			_iterator& operator+=(difference_type n) noexcept {
				_index += n;
				return *this;
			}

			constexpr _iterator operator+(difference_type n) const noexcept {
				return _iterator(_data, _offset, _index + n, _capacity);
			}

			_iterator& operator-=(difference_type n) noexcept {
				_index -= n;
				return *this;
			}

			constexpr _iterator operator-(difference_type n) const noexcept {
				assert(n <= _index);
				return _iterator(_data, _offset, _index - n, _capacity);
			}

			constexpr reference operator[](difference_type n) const {
				return *(*this + n);
			}

			constexpr friend difference_type operator-(_iterator const& lhs, _iterator const& rhs) noexcept {
				return lhs._index > rhs._index ? lhs._index - rhs._index : -(rhs._index - lhs._index);
			}

			constexpr friend _iterator operator+(difference_type lhs, _iterator const& rhs) noexcept {
				return rhs + lhs;
			}

			constexpr friend auto operator<=>(_iterator const& lhs, _iterator const& rhs) noexcept {
				return std::tie(lhs._data, lhs._offset, lhs._index) <=> std::tie(rhs._data, rhs._offset, rhs._index);
			}

			constexpr friend bool operator==(_iterator const& lhs, _iterator const& rhs) noexcept {
				return &*lhs == &*rhs;
			}

		private:
			pointer _data {};

			// Keeping both _offset and _index around is a little redundant,
			// algorithmically, but it makes it much easier to express iterator-mutating
			// operations.

			// Physical index of begin().
			size_type _offset {};
			// Logical index of this iterator.
			size_type _index {};

			size_type _capacity {};
		};

		using iterator = _iterator<pointer>;
		using const_iterator = _iterator<const_pointer>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		explicit RingBuffer(size_type capacity, allocator_type const& allocator) : _allocator(allocator) {
			reserve(capacity);
		}
		explicit RingBuffer(size_type capacity = 1) : RingBuffer(capacity, allocator_type {}) {}

		~RingBuffer() {
			clear();
			_deallocate();
		}

		RingBuffer(RingBuffer const& other)
			: RingBuffer(other, allocator_traits::select_on_container_copy_construction(other._allocator)) {}

		RingBuffer(RingBuffer const& other, allocator_type const& allocator) : RingBuffer(other._capacity, allocator) {
			clear();

			for (const_reference value : other) {
				push_back(value);
			}
		}

		RingBuffer(RingBuffer&& other) noexcept : RingBuffer(0, std::move(other._allocator)) {
			_no_alloc_swap(other);
		}

		RingBuffer(RingBuffer&& other, allocator_type const& allocator) : RingBuffer(0, allocator) {
			if (other._allocator == allocator) {
				_no_alloc_swap(other);
			} else {
				for (auto& element : other) {
					emplace_back(std::move(element));
				}
			}
		}

		RingBuffer& operator=(RingBuffer const& other) {
			clear();

			if constexpr (typename allocator_traits::propagate_on_container_copy_assignment()) {
				_allocator = other._allocator;
			}

			for (auto const& value : other) {
				push_back(value);
			}
			return *this;
		}

		RingBuffer& operator=(RingBuffer&& other) noexcept(
			allocator_traits::propagate_on_container_move_assignment::value ||
			std::is_nothrow_move_constructible<value_type>::value
		) {
			if (allocator_traits::propagate_on_container_move_assignment::value || _allocator == other._allocator) {
				// We're either getting the other's allocator or they're already the same,
				// so swap data in one go.
				if constexpr (typename allocator_traits::propagate_on_container_move_assignment()) {
					std::swap(_allocator, other._allocator);
				}
				_no_alloc_swap(other);
			} else {
				// Different allocators and can't swap them, so move elementwise.
				clear();
				for (auto& element : other) {
					emplace_back(std::move(element));
				}
			}

			return *this;
		}

		allocator_type get_allocator() const {
			return _allocator;
		}

	private:
		reference _unsafe_access(const size_type index) {
			return _data[_ring_wrap(_offset + index, capacity())];
		}
		const_reference _unsafe_access(const size_type index) const {
			return _data[_ring_wrap(_offset + index, capacity())];
		}

	public:
		reference front() {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return _unsafe_access(0);
		}
		reference back() {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _unsafe_access(size() - 1);
		}
		const_reference back() const {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _unsafe_access(size() - 1);
		}

		const_reference operator[](const size_type index) const {
			OV_HARDEN_ASSERT_ACCESS(index, "operator[]");
			return _unsafe_access(index);
		}
		reference operator[](const size_type index) {
			OV_HARDEN_ASSERT_ACCESS(index, "operator[]");
			return _unsafe_access(index);
		}

		const_reference at(const size_type index) const {
			if (OV_unlikely(index >= size())) {
				_abort_on_out_of_range("at", "index", index, size());
			}
			return _unsafe_access(index);
		}
		reference at(const size_type index) {
			if (OV_unlikely(index >= size())) {
				_abort_on_out_of_range("at", "index", index, size());
			}
			return _unsafe_access(index);
		}

		iterator begin() noexcept {
			return iterator(&_data[0], _offset, 0, _capacity);
		}
		iterator end() noexcept {
			return iterator(&_data[0], _offset, size(), _capacity);
		}
		const_iterator begin() const noexcept {
			return const_iterator(&_data[0], _offset, 0, _capacity);
		}
		const_iterator end() const noexcept {
			return const_iterator(&_data[0], _offset, size(), _capacity);
		}

		const_iterator cbegin() const noexcept {
			return const_cast<RingBuffer const&>(*this).begin();
		}
		const_iterator cend() const noexcept {
			return const_cast<RingBuffer const&>(*this).end();
		}

		reverse_iterator rbegin() noexcept {
			return reverse_iterator(end());
		}
		reverse_iterator rend() noexcept {
			return reverse_iterator(begin());
		}
		const_reverse_iterator rbegin() const noexcept {
			return const_reverse_iterator(end());
		}
		const_reverse_iterator rend() const noexcept {
			return const_reverse_iterator(begin());
		}

		const_reverse_iterator crbegin() const noexcept {
			return const_cast<RingBuffer const&>(*this).rbegin();
		}
		const_reverse_iterator crend() const noexcept {
			return const_cast<RingBuffer const&>(*this).rend();
		}

		bool empty() const noexcept {
			return size() == 0;
		}

		size_type size() const noexcept {
			return _size;
		}

		size_type capacity() const noexcept {
			return _capacity;
		}

		size_type max_size() const noexcept {
			return std::min(allocator_traits::max_size(_allocator), std::numeric_limits<ptrdiff_t>::max() / sizeof(value_type));
		}

		/// Calculates the space remaining inside the buffer before adding values will overwrite the front values.
		/// Unless size() is shrunk, once it returns 0, space() will always return 0.
		size_type space() const noexcept {
			return capacity() - size();
		}

		void reserve(size_type capacity) {
			if (OV_unlikely(_capacity != 0) && capacity <= _capacity) {
				return;
			}

			pointer result = _allocate(capacity);
			if (!empty()) {
				pointer last = std::uninitialized_copy_n(_data + _offset, this->capacity() + 1 - _offset, result);
				if (_offset > size()) {
					std::uninitialized_copy_n(_data, _offset - size(), last);
				}
				static_assert(std::is_destructible_v<value_type>, "value type is not destructible");
				if constexpr (!std::is_trivially_destructible_v<value_type>) {
					for (pointer first = _data; first != _data + _size; first++) {
						allocator_traits::destroy(_allocator, first);
					}
				}
			}
			_deallocate();
			_offset = 0;
			_next = size();
			_data = result;
			_capacity = capacity;
		}

		void resize(size_type count) {
			if (count == size()) {
				return;
			}

			if (size() > count) {
				erase(begin() + count, end());
				return;
			}

			reserve(count);
			std::uninitialized_default_construct_n(end(), count - size());
			_size = count;
			_next = _ring_wrap(_offset + _size, _capacity);
		}

		void resize(size_type count, value_type const& value) {
			if (count == size()) {
				return;
			}

			if (size() > count) {
				erase(begin() + count, end());
				return;
			}

			reserve(count);
			std::uninitialized_fill_n(end(), count - size(), value);
			_size = count;
			_next = _ring_wrap(_offset + _size, _capacity);
		}

		void shrink_to_fit() {
			if (_capacity > 0 && empty()) {
				_deallocate();
				_data = _allocate(0);
				_size = 0;
				_offset = 0;
				_next = 0;
				_capacity = 0;
				return;
			}

			size_type new_capacity = size();
			pointer result = _allocate(new_capacity);
			pointer last = std::uninitialized_copy_n(_data + _offset, capacity() + 1 - _offset, result);
			if (_offset > size()) {
				std::uninitialized_copy_n(_data, _offset - size(), last);
			}
			static_assert(std::is_destructible_v<value_type>, "value type is not destructible");
			if constexpr (!std::is_trivially_destructible_v<value_type>) {
				for (pointer first = _data; first != _data + _size; first++) {
					allocator_traits::destroy(_allocator, first);
				}
			}
			_deallocate();
			_offset = 0;
			_next = size();
			_data = result;
			_capacity = new_capacity;
		}

		void push_front(const_reference value) {
			emplace_front(value);
		}
		void push_front(value_type&& value) {
			emplace_front(std::move(value));
		}

		template<typename... Args>
		reference emplace_front(Args&&... args) {
			if (capacity() == 0) {
				// A buffer of size zero is conceptually sound, so let's support it.
				return (*this)[0];
			}

			allocator_traits::construct(_allocator, &_data[_decrement(_offset)], std::forward<Args>(args)...);

			// If required, make room for next time.
			if (size() == capacity()) {
				pop_back();
			}
			_grow_front();
			return (*this)[0];
		}

		void push_back(const_reference value) {
			emplace_back(value);
		}
		void push_back(value_type&& value) {
			emplace_back(std::move(value));
		}

		template<typename... Args>
		reference emplace_back(Args&&... args) {
			if (capacity() == 0) {
				// A buffer of size zero is conceptually sound, so let's support it.
				return (*this)[0];
			}

			allocator_traits::construct(_allocator, &_data[_next], std::forward<Args>(args)...);

			// If required, make room for next time.
			if (size() == capacity()) {
				pop_front();
			}
			_grow_back();
			return (*this)[size() - 1];
		}

		/// Appends the range between first (inclusive) and last (exclusive) to end().
		/// If size() + (last - first) > capacity(), rotates the front elements to the back and overwrites them.
		template<typename InputIt>
		iterator append(InputIt first, InputIt last) {
			using distance_type = typename std::iterator_traits<InputIt>::difference_type;

			const size_type _capacity = capacity();
			const distance_type distance = std::distance(first, last);
			if (OV_unlikely(distance <= 0)) {
				return end();
			}

			// Limit the number of elements to append at _capacity
			const size_type append_count = std::min<size_type>(distance, _capacity);

			// If appending would exceed capacity, remove elements from front
			const size_type excess = size() + append_count > _capacity ? size() + append_count - _capacity : 0;
			if (excess > 0) {
				// Destroy elements that will be overwritten
				if constexpr (!std::is_trivially_destructible_v<value_type>) {
					size_type pos = _offset;
					for (size_type i = 0; i < excess; ++i) {
						allocator_traits::destroy(_allocator, _data + pos);
						pos = _ring_wrap(pos + 1, _capacity);
					}
				}
				_offset = _ring_wrap(_offset + excess, _capacity);
				const size_type overflow_check = _size - excess;
				if (OV_unlikely(overflow_check > _size)) {
					_size = 0;
				} else {
					_size = overflow_check;
				}
			}

			// Determine physical position for appending
			const size_type write_pos = _ring_wrap(_offset + _size, _capacity);
			size_type space_to_end = _capacity - write_pos;

			iterator result = iterator(_data, _offset, _size, _capacity);
			if (OV_likely(append_count <= space_to_end)) {
				// Single copy to the back
				std::uninitialized_copy_n(first, append_count, _data + write_pos);
			} else {
				// Split copy: part to the end, part to the beginning
				++space_to_end;
				InputIt mid = first;
				std::advance(mid, space_to_end);
				std::uninitialized_copy(first, mid, _data + write_pos);
				std::uninitialized_copy_n(mid, append_count - space_to_end, _data);
			}

			_size += append_count;
			_next = _ring_wrap(_offset + _size, _capacity);
			return result;
		}

		/// Appends the range from first (inclusive) upto count to end().
		/// If size() + count > capacity(), rotates the front elements to the back and overwrites them.
		template<typename InputIt>
		iterator append(InputIt first, size_type count) {
			return append(first, first + count);
		}

		/// Appends the range to after end, truncated by write_size.
		/// If size() + (end(range) - begin(range)) > capacity(), rotates the front elements to the back and overwrites them
		template<ranges::input_range Range>
		iterator append_range(Range&& range, size_type write_size = std::numeric_limits<size_type>::max()) {
			if (write_size < capacity()) {
				auto end = ranges::begin(range);
				ranges::advance(end, std::min<size_type>(ranges::distance(range), write_size));
				return append(ranges::begin(range), end);
			}
			return append(ranges::begin(range), ranges::end(range));
		}

		/// Moves the front value and pops it off the buffer.
		/// Crashes if empty() is true and value_type is no default constructible.
		value_type read() {
			if (empty()) {
				if constexpr (std::is_default_constructible_v<value_type>) {
					return {};
				}
				std::abort();
			}
			value_type result = std::move((*this)[0]);
			pop_front();
			return result;
		}

		/// Moves the read_size of the buffer from the front into r_buffer and erases read_size from the buffer.
		/// If read_size exceeds capacity(), read_size will become capacity().
		std::span<value_type> read_buffer_to( //
			pointer r_buffer, size_type read_size = std::numeric_limits<size_type>::max()
		) {
			read_size = std::min(read_size, capacity());
			iterator last = begin();
			ranges::advance(last, read_size);
			ranges::move(std::make_move_iterator(begin()), std::make_move_iterator(last), r_buffer);
			erase(begin(), read_size);
			return std::span<value_type> { r_buffer, read_size };
		}

		/// Moves the read_size of the buffer to a vector from the front and erases read_size from the buffer.
		/// If read_size exceeds capacity(), read_size will become capacity().
		template<typename ReadAllocator = allocator_type>
		std::vector<value_type, ReadAllocator> read_buffer(size_type read_size) {
			read_size = std::min(read_size, capacity());
			iterator last = begin();
			ranges::advance(last, read_size);
			std::vector<value_type, ReadAllocator> result { std::make_move_iterator(begin()), std::make_move_iterator(last) };
			erase(begin(), read_size);
			return result;
		}

		/// Creates a subrange of iterator for read_size of the buffer.
		/// If read_size exceeds capacity(), read_size will become capacity().
		/// Note: Does not erase values inside the buffer.
		ranges::subrange<iterator> read_range(size_type read_size) {
			read_size = std::min(read_size, capacity());
			iterator last = begin();
			ranges::advance(last, read_size);
			return { begin(), last };
		}

		void pop_front() noexcept {
			if (empty()) {
				return;
			}

			if constexpr (!std::is_trivially_destructible_v<value_type>) {
				allocator_traits::destroy(_allocator, &_data[_offset]);
			}
			_shrink_front();
		}
		void pop_back() noexcept {
			if (empty()) {
				return;
			}

			_shrink_back();
			if constexpr (!std::is_trivially_destructible_v<value_type>) {
				allocator_traits::destroy(_allocator, &_data[_next]);
			}
		}

		void clear() noexcept {
			if constexpr (!std::is_trivially_destructible_v<value_type>) {
				if (empty()) {
					return;
				}
				for (pointer first = _data; first != _data + _size; first++) {
					allocator_traits::destroy(_allocator, first);
				}
			}
			_size = 0;
			_offset = 0;
			_next = 0;
		}

		iterator erase(const_iterator from, const_iterator to) noexcept(
			noexcept(pop_front()) && std::is_nothrow_move_assignable<value_type>::value
		) {
			if (OV_unlikely(from > end() || to > end())) {
				return std::bit_cast<iterator>(from);
			}

			if (from == to) {
				return iterator(_data, _offset, from - begin(), _capacity);
			}

			const difference_type erase_count = to - from;
			if (erase_count == 0) {
				return iterator(_data, _offset, from - begin(), _capacity);
			}

			const difference_type leading = from - begin();
			const difference_type trailing = end() - to;
			const size_type _capacity = capacity();

			iterator result = iterator(_data, _offset, from - begin(), _capacity);

			if (leading <= trailing) {
				// Shift elements from the front towards the erasure point
				const size_type dest_pos = _ring_wrap(_offset, _capacity);
				const size_type src_pos = _ring_wrap(_offset + erase_count, _capacity);
				size_type count = leading;

				// Move elements in one or two segments depending on wrap-around
				if (dest_pos <= src_pos || src_pos + leading <= _capacity + 1) {
					std::move(_data + src_pos, _data + src_pos + leading, _data + dest_pos);
				} else {
					size_type first_segment = _capacity + 1 - src_pos;
					std::move(_data + src_pos, _data + src_pos + first_segment, _data + dest_pos);
					std::move(_data, _data + (leading - first_segment), _data + dest_pos + first_segment);
				}

				// Destroy elements at the front
				if constexpr (!std::is_trivially_destructible_v<value_type>) {
					for (size_type i = 0; i < erase_count; ++i) {
						allocator_traits::destroy(_allocator, _data + _offset);
						_offset = _ring_wrap(_offset + 1, _capacity);
					}
				} else {
					_offset = _ring_wrap(_offset + erase_count, _capacity);
				}
				_size -= erase_count;
			} else if (trailing >= 0) {
				// Shift elements from the back towards the erasure point
				const size_type dest_pos = _ring_wrap(_offset + leading, _capacity);
				const size_type src_pos = _ring_wrap(_offset + leading + erase_count, _capacity);

				// Move elements in one or two segments depending on wrap-around
				if (dest_pos <= src_pos || src_pos + trailing <= _capacity + 1) {
					std::move(_data + src_pos, _data + src_pos + trailing, _data + dest_pos);
				} else {
					const size_type first_segment = _capacity + 1 - src_pos;
					std::move(_data + src_pos, _data + src_pos + first_segment, _data + dest_pos);
					std::move(_data, _data + (trailing - first_segment), _data + dest_pos + first_segment);
				}

				// Destroy elements at the back
				if constexpr (!std::is_trivially_destructible_v<value_type>) {
					for (size_type i = 0; i < erase_count; ++i) {
						_next = _ring_wrap(_next - 1, _capacity);
						allocator_traits::destroy(_allocator, _data + _next);
					}
				} else {
					if (erase_count < 0) {
						_next -= erase_count;
					} else {
						_next += erase_count;
					}
				}

				if (erase_count < 0) {
					_size += erase_count;
					_offset -= erase_count + 1;
				} else {
					_size -= erase_count;
				}
			}

			return result;
		}
		iterator erase(const_iterator pos, size_type count) noexcept(noexcept(erase(pos, pos + count))) {
			const_iterator last = pos;
			std::advance(last, count);
			return erase(pos, last);
		}
		iterator erase(const_iterator pos) noexcept(noexcept(erase(pos, 1))) {
			OV_HARDEN_ASSERT_VALID_ITERATOR(pos, "erase(const_iterator)");
			return erase(pos, 1);
		}

		void swap(RingBuffer& other) noexcept {
			if constexpr (typename allocator_traits::propagate_on_container_swap()) {
				std::swap(_allocator, other._allocator);
			}
			_no_alloc_swap(other);
		}

		friend auto operator<=>(RingBuffer const& lhs, RingBuffer const& rhs) {
			return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

		friend bool operator==(RingBuffer const& lhs, RingBuffer const& rhs) {
			if (lhs.size() != rhs.size()) {
				return false;
			}

			return std::equal(lhs.begin(), lhs.end(), rhs.begin());
		}

	private:
		void _no_alloc_swap(RingBuffer& other) {
			std::swap(_data, other._data);
			std::swap(_next, other._next);
			std::swap(_offset, other._offset);
			std::swap(_size, other._size);
			std::swap(_capacity, other._capacity);
		}

		size_type _increment(size_type& index, size_type amount = 1) {
			return index = (index + amount - 1U < capacity() ? index + amount : 0);
		}

		size_type _decrement(size_type& index, size_type amount = 1) {
			return index = ((index - amount) + 1U > 0 ? index - amount : capacity());
		}

		void _grow_front() {
			_decrement(_offset);
			++_size;
		}

		void _grow_back() {
			_increment(_next);
			++_size;
		}

		void _shrink_front() {
			_increment(_offset);
			--_size;
		}

		void _shrink_back() {
			_decrement(_next);
			--_size;
		}

		pointer _allocate(const size_type new_capacity) {
			return allocator_traits::allocate(_allocator, new_capacity + 1);
		}

		void _deallocate() {
			allocator_traits::deallocate(_allocator, _data, capacity() + 1);
		}

		static constexpr size_type _ring_wrap(const size_type ring_index, const size_type ring_capacity) {
			return (ring_index <= ring_capacity) ? ring_index : ring_index - ring_capacity - 1;
		}

		[[noreturn]] void _abort_on_out_of_range(const char* func_name, const char* var_name, size_t var, size_t size) const {
			OV_THROW_OUT_OF_RANGE("RingBuffer", func_name, var_name, var, size);
		}

		// The start of the dynamically allocated backing array.
		pointer _data = nullptr;
		// The next position to write to for push_back().
		size_type _next = 0U;

		// Start of the ring buffer in data_.
		size_type _offset = 0U;
		// The number of elements in the ring buffer (distance between begin() and
		// end()).
		size_type _size = 0U;
		// The capacity of the ring buffer
		size_type _capacity = 0U;

		// The allocator is used to allocate memory, and to construct and destroy
		// elements.
		[[no_unique_address]] allocator_type _allocator {};
	};
}
