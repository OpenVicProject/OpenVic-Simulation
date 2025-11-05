#pragma once

#include <algorithm>
#include <atomic>
#include <concepts>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include "openvic-simulation/core/Compare.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/container/Allocator.hpp"
#include "openvic-simulation/core/container/BasicIterator.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"

namespace OpenVic {
	/**
	 * A Copy-On-Write vector (replicating std::vector)
	 *
	 * Any unmodified copy refers to the same data in memory
	 * Any modified copy (calls write()) allocates a new memory pointer (given use_count() is not 1)
	 */
	template<typename T, typename Allocator = std::allocator<T>>
	class cow_vector {
		struct payload;

		using allocator_traits = std::allocator_traits<Allocator>;

		using mutable_pointer = std::allocator_traits<Allocator>::pointer;

		struct allocate_tag_t {};
		static constexpr allocate_tag_t allocate_tag {};

		OV_ALWAYS_INLINE cow_vector(allocate_tag_t, size_t reserve) : _data(_allocate_payload(reserve)) {}
		OV_ALWAYS_INLINE cow_vector(allocate_tag_t, Allocator const& alloc, size_t reserve)
			: alloc(alloc), _data(_allocate_payload(reserve)) {}

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::allocator_traits<allocator_type>::size_type;
		using difference_type = std::allocator_traits<allocator_type>::difference_type;
		using reference = value_type const&;
		using const_reference = reference;
		using pointer = std::allocator_traits<allocator_type>::const_pointer;
		using const_pointer = pointer;
		using iterator = basic_iterator<const_pointer, cow_vector>;
		using const_iterator = iterator;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		OV_ALWAYS_INLINE cow_vector() : cow_vector(allocate_tag, 0) {}
		OV_ALWAYS_INLINE explicit cow_vector(Allocator const& alloc) : cow_vector(allocate_tag, alloc, 0) {}

		OV_ALWAYS_INLINE explicit cow_vector(size_type count, Allocator const& alloc) : cow_vector(allocate_tag, alloc, count) {
			_data->array_end = uninitialized_default_n(_data->array, count, this->alloc);
		}

		OV_ALWAYS_INLINE cow_vector(size_type count, const T& value, Allocator const& alloc = Allocator())
			: cow_vector(count, alloc) {
			_data->array_end = uninitialized_fill_n(_data->array, count, value, this->alloc);
		}

		template<class InputIt>
		OV_ALWAYS_INLINE cow_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
			: cow_vector(allocate_tag, alloc, _validate_iterator_difference(last - first)) {
			_data->array_end = uninitialized_copy(first, last, _data->array, this->alloc);
		}

		OV_ALWAYS_INLINE cow_vector(cow_vector const& other)
			: alloc(allocator_traits::select_on_container_copy_construction(other.alloc)), _data(other._data) {
			if (_data) {
				++_data->count;
			}
		}

		OV_ALWAYS_INLINE cow_vector(cow_vector&& other) : alloc(std::move(other.alloc)), _data(other._data) {
			other._data = nullptr;
		}

		OV_ALWAYS_INLINE cow_vector(cow_vector const& other, std::type_identity_t<Allocator> const& alloc)
			: cow_vector(other.begin(), other.end(), alloc) {}

		OV_ALWAYS_INLINE cow_vector(cow_vector&& other, std::type_identity_t<Allocator> const& alloc) : alloc(alloc) {
			if constexpr (allocator_traits::is_always_equal::value) {
				swap(other, *this);
			} else {
				if (other.get_allocator() == alloc) {
					swap(other, *this);
				} else if (!other.empty()) {
					_data = _allocate_payload(other.size());
					_data->array_end = uninitialized_move(other._data->array, other._data->array_end, _data->array, alloc);
					destroy(_data->array, _data->array_end, alloc);
					_data->array_end = _data->array;
				}
			}
		}

		OV_ALWAYS_INLINE cow_vector(std::initializer_list<T> init, Allocator const& alloc = Allocator())
			: cow_vector(init.begin(), init.end(), alloc) {}

		OV_ALWAYS_INLINE ~cow_vector() {
			if (_data && --_data->count == 0) {
				destroy(_data->array, _data->array_end, alloc);
				_deallocate_payload(_data);
				_data = nullptr;
			}
		}

		cow_vector& operator=(cow_vector const& x) {
			if (&x != this) {
				*this = cow_vector(x);
			}
			return *this;
		}

		cow_vector& operator=(cow_vector&& x) {
			writer& self_writer = *reinterpret_cast<writer*>(this);
			cow_vector tmp = std::move(x);
			writer& tmp_writer = *reinterpret_cast<writer*>(&tmp);

			if constexpr (allocator_traits::propagate_on_container_move_assignment::value ||
						  allocator_traits::is_always_equal::value) {
				self_writer.swap(tmp_writer);
			} else if (alloc == x.alloc) {
				self_writer.swap(tmp_writer);
			} else {
				// If allocators are different instances, only resolution is a move, tmp dies
				self_writer = this->write();
				tmp_writer = tmp.write();

				self_writer._assign_aux(
					std::make_move_iterator(tmp_writer.begin()), std::make_move_iterator(tmp_writer.end()),
					std::random_access_iterator_tag()
				);
				tmp_writer.clear();
			}
			return *this;
		}

		cow_vector& operator=(std::initializer_list<value_type> ilist) {
			*this = cow_vector(ilist, alloc);
			return *this;
		}

		void assign(size_type count, T const& value) {
			*this = cow_vector(count, value, alloc);
		}

		template<class InputIt>
		void assign(InputIt first, InputIt last) {
			*this = cow_vector(first, last, alloc);
		}

		void assign(std::initializer_list<T> ilist) {
			*this = cow_vector(ilist, alloc);
		}

		allocator_type get_allocator() const {
			return alloc;
		}

		const_reference at(size_type pos) const {
			// TODO: crash on boundary violation
			return (*this)[pos];
		}

		const_reference operator[](size_type pos) const {
			return _data->array[pos];
		}

		const_reference front() const {
			return *_data->array;
		}

		const_reference back() const {
			return *_data->array_end;
		}

		T const* data() const {
			return !_data ? nullptr : _data->array;
		}

		std::span<const T> read() const {
			if (!_data) {
				return {};
			}

			return { _data->array, _data->array_end };
		}

		const_iterator begin() const {
			return !_data ? end() : const_iterator(_data->array);
		}

		const_iterator cbegin() const {
			return !_data ? cend() : begin();
		}

		const_iterator end() const {
			return !_data ? const_iterator() : const_iterator(_data->array_end);
		}

		const_iterator cend() const {
			return end();
		}

		const_reverse_iterator rbegin() const {
			return !_data ? rend() : const_reverse_iterator(_data->array);
		}

		const_reverse_iterator crbegin() const {
			return rbegin();
		}

		const_reverse_iterator rend() const {
			return !_data ? const_reverse_iterator() : const_reverse_iterator(_data->array_end);
		}

		const_reverse_iterator crend() const {
			return rend();
		}

		bool empty() const {
			return !_data || _data->array == _data->array_end;
		}

		size_type size() const {
			if (!_data) {
				return 0ul;
			}

			difference_type dif = _data->array_end - _data->array;
			if (dif < 0) {
				unreachable();
			}
			return size_type(dif);
		}

		size_type max_size() const {
			const size_t diffmax = std::numeric_limits<ptrdiff_t>::max() / sizeof(T);
			const size_t allocmax = allocator_traits::max_size(alloc);
			return std::min(diffmax, allocmax);
		}

		size_type capacity() const {
			if (!_data) {
				return 0ul;
			}

			difference_type dif = _data->store_end - _data->array;
			if (dif < 0) {
				unreachable();
			}
			return size_type(dif);
		}

		size_t use_count(std::memory_order memory_order = std::memory_order::relaxed) const {
			return !_data ? 0 : _data->count.load(memory_order);
		}

		friend struct writer;
		struct writer;

		writer& write() {
			if (!_data || _data->count <= 1) {
				return *reinterpret_cast<writer*>(this);
			}

			writer& result = write_for_overwrite();
			result._data->array_end = uninitialized_copy(_data->array, _data->array_end, result._data->array, alloc);
			return result;
		}

		writer& write_for_overwrite() {
			writer& result = *reinterpret_cast<writer*>(this);
			if (!_data || _data->count <= 1) {
				return result;
			}

			--_data->count;
			result._data = result._allocate_payload(size());
			return result;
		}

	private:
		struct payload {
			std::atomic_size_t count = 1;
			mutable_pointer store_end = nullptr;
			mutable_pointer array_end = nullptr;
			T* array;

			static const size_type content_size;

			payload(size_t capacity) : array(&reinterpret_cast<T*>(this)[payload::content_size]) {
				array_end = array;
				store_end = array + capacity;
			}
		};

		OV_NO_UNIQUE_ADDRESS allocator_type alloc;
		mutable payload* _data;

		inline payload* _allocate_payload(size_t reserve) {
			if (reserve == 0) {
				return nullptr;
			}

			// TODO: crash on reserve bigger then max alloc size

			payload* result = reinterpret_cast<payload*>(allocator_traits::allocate(alloc, payload::content_size + reserve));
			std::construct_at(result, reserve);
			return result;
		}

		inline void _deallocate_payload(payload* data) {
			if (!data) {
				return;
			}

			size_type vec_size = size_type(data->store_end - data->array);
			allocator_traits::deallocate(alloc, reinterpret_cast<T*>(data), payload::content_size + vec_size);
		}

		template<typename InputIt>
		inline difference_type _validate_iterator_difference(InputIt first, InputIt last) {
			difference_type result = last - first;
			// TODO: crash on negative result
			return result;
		}

		struct for_overwrite_t {};
		static constexpr for_overwrite_t for_overwrite {};
	};

	template<typename T, typename Allocator>
	struct cow_vector<T, Allocator>::writer : public cow_vector {
		using value_type = T;
		using allocator_type = Allocator;
		using size_type = std::allocator_traits<allocator_type>::size_type;
		using difference_type = std::allocator_traits<allocator_type>::difference_type;
		using reference = value_type&;
		using const_reference = cow_vector::const_reference;
		using pointer = std::allocator_traits<allocator_type>::pointer;
		using const_pointer = cow_vector::const_pointer;
		using iterator = basic_iterator<pointer, cow_vector>;
		using const_iterator = cow_vector::const_iterator;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		void assign(size_type count, T const& value) {
			_fill_assign(count, value);
		}

		template<class InputIt>
		void assign(InputIt first, InputIt last) {
			_assign_aux(first, last, std::iterator_traits<decltype(first)>::iterator_category());
		}

		void assign(std::initializer_list<T> ilist) {
			_assign_aux(ilist.begin(), ilist.end(), std::random_access_iterator_tag());
		}

		reference at(size_type pos) {
			return (*this)[pos];
		}

		reference operator[](size_type pos) {
			// TODO: crash on boundary violation
			return _data->array[pos];
		}

		reference front() {
			return *_data->array;
		}

		reference back() {
			return *_data->array_end;
		}

		T* data() {
			return !_data ? nullptr : _data->array;
		}

		iterator begin() {
			return !_data ? end() : iterator(_data->array);
		}

		iterator end() {
			return !_data ? iterator() : iterator(_data->array_end);
		}

		reverse_iterator rbegin() {
			return !_data ? rend() : reverse_iterator(_data->array);
		}

		reverse_iterator rend() {
			return !_data ? reverse_iterator() : reverse_iterator(_data->array_end);
		}

		void reserve(size_type count) {
			if (count > max_size()) {
				// TODO: crash
			}
			if (capacity() >= count) {
				return;
			}
			const size_type old_size = size();
			payload* new_data = _allocate_payload(count);
			if (_data) {
				if constexpr (move_insertable_allocator<allocator_type>) {
					_relocate(_data->array, _data->array_end, new_data->array, alloc);
				} else {
					uninitialized_move(_data->array, _data->array_end, new_data->array, alloc);
					destroy(_data->array, _data->array_end, alloc);
				}
				_deallocate_payload(_data);
			}
			_set_payload(new_data, new_data->array + old_size);
		}

		void shrink_to_fit() {
			// GCC's implementation disables shrink_to_fit if exceptions are disabled, prefer ignorance
			if constexpr (std::copy_constructible<value_type> || std::move_constructible<value_type>) {
				if (empty()) {
					return;
				}
				const size_type new_cap = size();
				payload* new_data = _allocate_payload(new_cap);
				if (new_data) {
					if constexpr (move_insertable_allocator<allocator_type>) {
						_relocate(_data->array, _data->array_end, new_data->array, alloc);
					} else {
						new_data->array_end = uninitialized_move(_data->array, _data->array_end, new_data->array, alloc);
						destroy(_data->array, _data->array_end, alloc);
					}
					_deallocate_payload(_data);
				}
				_data = new_data;
			}
		}

		void clear() {
			if (empty()) {
				return;
			}

			destroy(_data->array, _data->array_end, alloc);
			_data->array_end = _data->array;
		}

		iterator insert(const_iterator pos, T const& value) {
			const size_type n = pos - begin();
			if (_data) {
				if (_data->array_end != _data->store_end) {
					if (pos == const_iterator()) {
						unreachable();
					}

					if (pos == end()) {
						allocator_traits::construct(alloc, _data->array_end, value);
						++_data->array_end;
					} else {
						const auto p = begin() + (pos - cbegin());
						temp_value copy(this, value);
						_insert_aux(p, std::move(copy.value()));
					}
				} else {
					_realloc_insert(begin() + (pos - cbegin()), value);
				}
			} else {
				_alloc_append(value);
			}

			return iterator(_data->array + n);
		}

		iterator insert(const_iterator pos, T&& value) {
			return _insert_rval(pos, std::move(value));
		}

		iterator insert(const_iterator pos, size_type count, T const& value) {
			difference_type offset = pos - cbegin();
			_fill_insert(begin() + offset, count, value);
			return begin() + offset;
		}

		template<class InputIt>
		iterator insert(const_iterator pos, InputIt first, InputIt last) {
			difference_type offset = pos - cbegin();
			_range_insert(begin() + offset, first, last, std::iterator_traits<decltype(first)>::iterator_category());
			return begin() + offset;
		}

		iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
			auto offset = pos - cbegin();
			_range_insert(begin() + offset, ilist.begin(), ilist.end(), std::random_access_iterator_tag());
			return begin() + offset;
		}

		template<class... Args>
		iterator emplace(const_iterator pos, Args&&... args) {
			return _emplace_aux(pos, std::forward<Args>(args)...);
		}

		iterator erase(const_iterator pos) {
			if (pos + 1 != end()) {
				std::move(pos + 1, end(), pos);
			}
			--_data->array_end;
			allocator_traits::destroy(alloc, _data->array_end);
			return pos;
		}

		iterator erase(const_iterator first, const_iterator last) {
			if (first != last) {
				if (last != end()) {
					std::move(last, end(), first);
				}
				_erase_at_end(first.base() + (end() - last));
			}
			return first;
		}

		void push_back(T const& value) {
			if (_data) {
				if (_data->array_end != _data->store_end) {
					allocator_traits::construct(alloc, _data->array_end, value);
					++_data->array_end;
				} else {
					_realloc_append(value);
				}
			} else {
				_alloc_append(value);
			}
		}

		void push_back(T&& value) {
			emplace_back(std::move(value));
		}

		template<class... Args>
		reference emplace_back(Args&&... args) {
			if (_data) {
				if (_data->array_end != _data->store_end) {
					allocator_traits::construct(alloc, _data->array_end, std::forward<Args>(args)...);
					++_data->array_end;
				} else {
					_realloc_append(std::forward<Args>(args)...);
				}
			} else {
				_alloc_append(std::forward<Args>(args)...);
			}
			return back();
		}

		void pop_back() {
			// TODO: assert if empty
			--_data->array_end;
			allocator_traits::destroy(alloc, _data->array_end);
		}

		void resize(size_type count) {
			if (count > size()) {
				_default_append(count - size());
			} else if (count < size()) {
				_erase_at_end(_data->array + count);
			}
		}

		void resize(size_type count, value_type const& value) {
			if (!_data) {
				_fill_assign(count, value);
			} else if (count > size()) {
				_fill_insert(end(), count - size(), value);
			} else if (count < size()) {
				_erase_at_end(_data->array + count);
			}
		}

		void swap(writer& other) {
			// TODO: allocator_traits::propagate_on_container_swap::value || alloc == other.alloc assert
			payload* tmp = _data;
			_data = other._data;
			other._data = tmp;
			if constexpr (allocator_traits::propagate_on_container_swap::value) {
				using std::swap;
				swap(alloc, other.alloc);
			}
		}

	private:
		writer() = delete;
		explicit writer(Allocator const& alloc) = delete;
		explicit writer(size_type count, Allocator const& alloc) = delete;
		writer(size_type count, const T& value, Allocator const& alloc = Allocator()) = delete;
		template<class InputIt>
		writer(InputIt first, InputIt last, const Allocator& alloc = Allocator()) = delete;
		writer(cow_vector const& other) = delete;
		writer(cow_vector&& other) = delete;
		writer(cow_vector const& other, std::type_identity_t<Allocator> const& alloc) = delete;
		writer(cow_vector&& other, std::type_identity_t<Allocator> const& alloc) = delete;
		writer(std::initializer_list<T> init, Allocator const& alloc = Allocator()) = delete;

		cow_vector& operator=(cow_vector const& x) = delete;
		cow_vector& operator=(cow_vector&& x) = delete;
		cow_vector& operator=(std::initializer_list<value_type> ilist) = delete;

		friend cow_vector;
		static pointer _relocate(pointer first, pointer last, pointer result, allocator_type& alloc) {
			if constexpr (trivially_relocatable<T>) {
				difference_type dif = last - first;
				if (dif > 0) {
					std::memcpy(result, first, dif * sizeof(T));
				}
				return result + dif;
			} else {
				pointer cur = result;
				for (; first != last; ++first, ++cur) {
					allocator_traits::construct(alloc, cur, std::move(*first));
					allocator_traits::destroy(alloc, first);
				}
				return cur;
			}
		}

		struct guard_alloc {
			payload* data;
			cow_vector& vector;

			guard_alloc(payload* data, cow_vector& vec) : data(data), vector(vec) {}

			~guard_alloc() {
				if (data) {
					vector._deallocate_payload(data);
				}
			}

			pointer release() {
				pointer result = data;
				data = pointer();
				return result;
			}

		private:
			guard_alloc(const guard_alloc&);
		};

		struct guard_elements {
			pointer first, last; // Elements to destroy
			allocator_type& alloc;

			guard_elements(pointer elt, size_type n, allocator_type& a) : first(elt), last(elt + n), alloc(a) {}

			~guard_elements() {
				destroy(first, last, alloc);
			}

		private:
			guard_elements(const guard_elements&);
		};

		OV_ALWAYS_INLINE void _set_payload(payload* data, pointer array_end) {
			_data = data;
			_data->array_end = array_end;
		}

		template<typename... Args>
		void _realloc_insert(iterator pos, Args&&... args) {
			// TODO: crash if can't allocate 1
			const size_type len = size() + 1;
			payload* old_data = _data;
			pointer old_start = _data->array;
			pointer old_finish = _data->array_end;
			const size_type elems_before = pos - begin();
			payload* new_data = _allocate_payload(len);
			pointer new_start = new_data->array;
			pointer new_finish = new_start;

			{
				guard_alloc guard(new_data, *this);

				allocator_traits::construct(alloc, std::to_address(new_start + elems_before), std::forward<Args>(args)...);

				if constexpr (move_insertable_allocator<allocator_type>) {
					new_finish = _relocate(old_start, pos.base(), new_start, alloc);
					++new_finish;
					new_finish = _relocate(pos.base(), old_finish, new_finish, alloc);
				} else {
					// Guard the new element so it will be destroyed if anything throws.
					guard_elements guard_elts(new_start + elems_before, 1, alloc);

					new_finish = uninitialized_move(old_start, pos.base(), new_start, alloc);

					++new_finish;
					// Guard everything before the new element too.
					guard_elts.first = new_start;

					new_finish = uninitialized_move(pos.base(), old_finish, new_finish, alloc);

					// New storage has been fully initialized, destroy the old elements.
					guard_elts.first = old_start;
					guard_elts.last = old_finish;
				}
				guard.data = _data;
			}
			_set_payload(new_data, new_finish);
		}

		struct temp_value {
			template<typename... Args>
			explicit temp_value(cow_vector* vec, Args&&... args) : self(vec) {
				allocator_traits::construct(self->alloc, ptr(), std::forward<Args>(args)...);
			}

			~temp_value() {
				allocator_traits::destroy(self->alloc, ptr());
			}

			value_type& value() {
				return storage.value;
			}

		private:
			T* ptr() {
				return std::addressof(storage.value);
			}

			union storage_t {
				constexpr storage_t() : byte() {}
				~storage_t() {}
				storage_t& operator=(const storage_t&) = delete;
				unsigned char byte;
				T value;
			};

			cow_vector* self;
			storage_t storage;
		};

		template<typename Arg>
		void _insert_aux(iterator pos, Arg&& arg) {
			allocator_traits::construct(alloc, _data->array_end, std::move(*(_data->array_end - 1)));
			++_data->array_end;
			std::move_backward(pos.base(), _data->array_end - 2, _data->array_end - 1);
			*pos = std::forward<Arg>(arg);
		}

		template<typename It, typename ItTag>
		void _range_insert(iterator pos, It first, It last, ItTag) {
			if constexpr (std::derived_from<ItTag, std::forward_iterator_tag>) {
				if (first != last) {
					const size_type n = std::distance(first, last);
					if (size_type(_data->store_end - _data->array_end) >= n) {
						const size_type elems_after = end() - pos;
						pointer old_finish = _data->array_end;
						if (elems_after > n) {
							uninitialized_move(_data->array_end - n, _data->array_end, _data->array_end, alloc);
							_data->array_end += n;
							std::move_backward(pos.base(), old_finish - n, old_finish);
							std::copy(first, last, pos);
						} else {
							It mid = first;
							std::advance(mid, elems_after);
							uninitialized_copy(mid, last, _data->array_end, alloc);
							_data->array_end += n - elems_after;
							uninitialized_move(pos.base(), old_finish, _data->array_end, alloc);
							_data->array_end += elems_after;
							std::copy(first, mid, pos);
						}
					} else {
						// Make local copies of these members because the compiler
						// thinks the allocator can alter them if 'this' is globally
						// reachable.
						pointer old_start = _data->array;
						pointer old_finish = _data->array_end;

						const size_type len = n;
						// TODO: crash if can't allocate n

						payload* new_data = _allocate_payload(len);
						pointer new_start = new_data->array;
						pointer new_finish = new_start;
						new_finish = uninitialized_move(old_start, pos.base(), new_start, alloc);
						new_finish = uninitialized_copy(first, last, new_finish, alloc);
						new_finish = uninitialized_move(pos.base(), old_finish, new_finish, alloc);
						destroy(old_start, old_finish, alloc);
						_deallocate_payload(_data);
						_set_payload(new_data, new_finish);
					}
				}
			} else {
				if (pos == end()) {
					for (; first != last; ++first) {
						insert(end(), *first);
					}
				} else if (first != last) {
					std::vector<T, allocator_type> tmp(first, last, alloc);
					insert(pos, std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
				}
			}
		}

		void _fill_insert(iterator pos, size_type n, const value_type& value) {
			if (n != 0) {
				if (size_type(_data->store_end - _data->array_end) >= n) {
					temp_value tmp(this, value);
					value_type& copy = tmp.value();
					const size_type elems_after = end() - pos;
					pointer old_finish = _data->array_end;
					if (elems_after > n) {
						uninitialized_move(old_finish - n, old_finish, old_finish, alloc);
						_data->array_end += n;
						std::move_backward(pos.base(), old_finish - n, old_finish);
						std::fill(pos.base(), pos.base() + n, copy);
					} else {
						_data->array_end = uninitialized_fill_n(old_finish, n - elems_after, copy, alloc);
						uninitialized_move(pos.base(), old_finish, _data->array_end, alloc);
						_data->array_end += elems_after;
						std::fill(pos.base(), old_finish, copy);
					}
				} else {
					// Make local copies of these members because the compiler thinks
					// the allocator can alter them if 'this' is globally reachable.
					pointer old_start = _data->array;
					pointer old_finish = _data->array_end;
					const pointer p = pos.base();

					const size_type len = n;
					// TODO: crash if can't allocate n
					const size_type elems_before = p - old_start;
					payload* new_data = _allocate_payload(len);
					pointer new_start = new_data->array;
					pointer new_finish = new_start;
					// See _M_realloc_insert above.
					uninitialized_fill_n(new_start + elems_before, n, value, alloc);
					new_finish = pointer();

					new_finish = uninitialized_move(old_start, p, new_start, alloc);
					new_finish += n;
					new_finish = uninitialized_move(p, old_finish, new_finish, alloc);

					destroy(old_start, old_finish, alloc);
					_deallocate_payload(_data);
					_set_payload(new_data, new_finish);
				}
			}
		}

		iterator _insert_rval(const_iterator pos, value_type&& value) {
			const auto n = pos - cbegin();
			if (_data) {
				if (_data->array_end != _data->store_end) {
					if (pos == cend()) {
						allocator_traits::construct(alloc, _data->array_end, std::move(value));
						++_data->array_end;
					} else {
						_insert_aux(begin() + n, std::move(value));
					}
				} else {
					_realloc_insert(begin() + n, std::move(value));
				}
			} else {
				_alloc_append(std::move(value));
			}

			return iterator(_data->array + n);
		}

		template<typename... Args>
		iterator _emplace_aux(const_iterator pos, Args&&... args) {
			const auto n = pos - cbegin();
			if (_data) {
				if (_data->array_end != _data->store_end) {
					if (pos == cend()) {
						allocator_traits::construct(alloc, _data->array_end, std::forward<Args>(args)...);
						++_data->array_end;
					} else {
						// We need to construct a temporary because something in args...
						// could alias one of the elements of the container and so we
						// need to use it before _M_insert_aux moves elements around.
						temp_value tmp(this, std::forward<Args>(args)...);
						_insert_aux(begin() + n, std::move(tmp._M_val()));
					}
				} else {
					_realloc_insert(begin() + n, std::forward<Args>(args)...);
				}
			} else {
				_alloc_append(std::forward<Args>(args)...);
			}

			return iterator(_data->array + n);
		}

		iterator _emplace_aux(const_iterator pos, value_type&& value) {
			return _insert_rval(pos, std::move(value));
		}

		void _erase_at_end(pointer pos) {
			if (size_type n = _data->array_end - pos) {
				destroy(pos, _data->array_end, alloc);
				_data->array_end = pos;
			}
		}

		template<typename... Args>
		void _alloc_append(Args&&... args) {
			const size_type len = 1;
			// TODO: crash if can't allocate 1
			_data = _allocate_payload(len);
			allocator_traits::construct(alloc, _data->array, std::forward<Args>(args)...);
			++_data->array_end;
		}

		template<typename... Args>
		void _realloc_append(Args&&... args) {
			const size_type len = 1;
			// TODO: crash if can't allocate 1
			pointer old_start = _data->array;
			pointer old_finish = _data->array_end;
			const size_type elems = end() - begin();
			payload* new_data = _allocate_payload(len);
			pointer new_start = new_data->array;
			pointer new_finish = new_start;

			{
				guard_alloc guard(new_start, len, *this);

				allocator_traits::construct(alloc, std::to_address(new_start + elems), std::forward<Args>(args)...);

				if constexpr (move_insertable_allocator<allocator_type>) {
					new_finish = _relocate(old_start, old_finish, new_start, alloc);
					++new_finish;
				} else {
					// Guard the new element so it will be destroyed if anything throws.
					guard_elements guard_elts(new_start + elems, 1, alloc);

					new_finish = uninitialized_move(old_start, old_finish, new_start, alloc);

					++new_finish;

					// New storage has been fully initialized, destroy the old elements.
					guard_elts.first = old_start;
					guard_elts.last = old_finish;
				}
				guard.data = _data;
			}
			_set_payload(new_data, new_finish);
		}

		void _default_append(size_type n) {
			if (n == 0) {
				return;
			}

			if (!_data) {
				const size_type len = n;
				// TODO: crash if can't allocate n

				_data = _allocate_payload(len);
				_data->array_end = uninitialized_default_n(_data->array_end, n, alloc);
				return;
			}


			const size_type _size = size();
			size_type navail = size_type(_data->store_end - _data->array_end);

			if (_size > max_size() || navail > max_size() - _size) {
				unreachable();
			}

			if (navail >= n) {
				if (!_data->array_end) {
					unreachable();
				}

				_data->array_end = uninitialized_default_n(_data->array_end, n, alloc);
			} else {
				// Make local copies of these members because the compiler thinks
				// the allocator can alter them if 'this' is globally reachable.
				pointer old_start = _data->array;
				pointer old_finish = _data->array_end;

				const size_type len = n;
				// TODO: crash if can't allocate n

				payload* new_data = _allocate_payload(len);
				pointer new_start = new_data->array;

				{
					guard_alloc guard(new_start, len, *this);

					uninitialized_default_n(new_start + _size, n, alloc);

					if constexpr (move_insertable_allocator<allocator_type>) {
						_relocate(old_start, old_finish, new_start, alloc);
					} else {
						guard_elements guard_elts(new_start + _size, n, alloc);

						uninitialized_move(old_start, old_finish, new_start, alloc);

						guard_elts.first = old_start;
						guard_elts.last = old_finish;
					}
					guard.data = _data;
				}
				_set_payload(new_data, new_start + _size + n);
			}
		}

		void _fill_assign(size_t n, const value_type& value) {
			const size_type _size = size();
			if (n > capacity()) {
				if (n <= _size) {
					unreachable();
				}

				cow_vector<T, allocator_type> tmp(n, value, alloc);
				_data = tmp._data;
				tmp._data = nullptr;
			} else if (n > _size) {
				std::fill(begin(), end(), value);
				const size_type add = n - _size;
				_data->array_end = uninitialized_fill_n(_data->array_end, add, value, alloc);
			} else {
				_erase_at_end(std::fill_n(_data->array, n, value));
			}
		}

		template<typename It, typename ItTag>
		void _assign_aux(It first, It last, ItTag) {
			if constexpr (std::derived_from<ItTag, std::forward_iterator_tag>) {
				const size_type _size = size();
				const size_type len = std::distance(first, last);

				if (len > capacity()) {
					if (len <= _size) {
						unreachable();
					}

					// TODO: crash if can't allocate len

					payload new_data = _allocate_payload(len);
					{
						guard_alloc guard(new_data, *this);
						uninitialized_copy(first, last, guard.data->array, alloc);
						guard.release();
					}

					destroy(_data->array, _data->array_end, alloc);
					_deallocate_payload(_data);
					_set_payload(new_data, new_data->array + len);
				} else if (_size >= len) {
					_erase_at_end(std::copy(first, last, _data->array));
				} else {
					It mid = first;
					std::advance(mid, _size);
					std::copy(first, mid, _data->array);
					[[maybe_unused]] const size_type n = len - _size;
					_data->array_end = uninitialized_copy(mid, last, _data->array_end, alloc);
				}
			} else if (_data) {
				pointer cur = _data->array;
				for (; first != last && cur != _data->array_end; ++cur, (void)++first) {
					*cur = *first;
				}
				if (first == last) {
					_erase_at_end(cur);
				} else {
					_range_insert(end(), first, last, std::iterator_traits<decltype(first)>::iterator_category());
				}
			} else {
				_data = _allocate_payload(std::distance(first, last));
				_range_insert(end(), first, last, std::iterator_traits<decltype(first)>::iterator_category());
			}
		}
	};

	template<typename T, typename Allocator>
	inline constexpr cow_vector<T, Allocator>::size_type cow_vector<T, Allocator>::payload::content_size =
		std::max<size_type>(1ul, (sizeof(payload) - sizeof(array)) / sizeof(T));

	template<typename T, typename Allocator>
	[[nodiscard]] inline bool operator==(cow_vector<T, Allocator> const& x, cow_vector<T, Allocator> const& y) {
		return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
	}

	template<typename T, typename Allocator>
	[[nodiscard]] auto operator<=>(cow_vector<T, Allocator> const& x, cow_vector<T, Allocator> const& y) {
		return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(), three_way_compare);
	}

	namespace cow {
		template<specialization_of<cow_vector> T>
		T const& read(T& v) {
			return v;
		}

		template<specialization_of<cow_vector> T>
		typename T::writer& write(T& v) {
			return v.write();
		}
	}

	static_assert(
		sizeof(cow_vector<int>) == sizeof(cow_vector<int>::writer), "cow_vector must always be the same size as it's writer"
	);
}

namespace std {
	template<typename T, typename Allocator>
	inline void swap( //
		typename OpenVic::cow_vector<T, Allocator>::writer& x, typename OpenVic::cow_vector<T, Allocator>::writer& y
	) {
		x.swap(y);
	}

	template<typename T, typename Allocator, typename Predicate>
	inline typename OpenVic::cow_vector<T, Allocator>::size_type erase_if( //
		typename OpenVic::cow_vector<T, Allocator>::writer& cont, Predicate pred
	) {
		using namespace OpenVic;
		typename cow_vector<T, Allocator>::writer& ucont = cont;
		const auto orig_size = cont.size();
		const auto end = ucont.end();
		decltype(end) removed = std::remove_if(ucont.begin(), end, std::ref(pred));
		if (removed != end) {
			cont.erase(unwrap_iterator(cont.begin(), removed), cont.end());
			return orig_size - cont.size();
		}

		return 0;
	}

	template<typename T, typename Allocator, typename U>
	inline typename OpenVic::cow_vector<T, Allocator>::size_type erase( //
		typename OpenVic::cow_vector<T, Allocator>::writer& cont, U const& value
	) {
		using namespace OpenVic;
		typename cow_vector<T, Allocator>::writer& ucont = cont;
		const auto orig_size = cont.size();
		const auto end = ucont.end();
		decltype(end) removed = std::remove_if(ucont.begin(), end, [&value](auto it) {
			*it == value;
		});
		if (removed != end) {
			cont.erase(unwrap_iterator(cont.begin(), removed), cont.end());
			return orig_size - cont.size();
		}

		return 0;
	}
}
