#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

#include "openvic-simulation/utility/Concepts.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

namespace OpenVic::_detail {
	//fixed capacity + not movable + not copyable
	template <typename T, typename Allocator = std::allocator<T>>
	class FixedVector {
	private:
		using allocator_traits = std::allocator_traits<Allocator>;
		const size_t _max_size;
		size_t _size;
		OV_NO_UNIQUE_ADDRESS Allocator _allocator;
		T* const _data_start_ptr;

	public:
		using const_reference = T const&;
		using difference_type = ptrdiff_t;
		using reference = T&;
		using size_type = size_t;
		using value_type = T;

		constexpr size_t size() const { return _size; }
		constexpr size_t capacity() const { return _max_size; }
		constexpr size_t max_size() const { return _max_size; }
		constexpr T* data() { return _data_start_ptr; }
		constexpr T const* data() const { return _data_start_ptr; }
		constexpr bool empty() const { return _size == 0; }

		explicit FixedVector(const size_t capacity)
			: _max_size(capacity),
			_size(0),
			_allocator(),
			_data_start_ptr(allocator_traits::allocate(_allocator, capacity)) {}

		//Generator (size_t i) -> U (where T is constructable from U)
		template<typename GeneratorTemplateType>
		// The generator must NOT return a tuple
		requires (!specialization_of<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_t>>, std::tuple>)
		// The type must be constructible from the generator's single return value
		&& std::constructible_from<T, decltype(std::declval<GeneratorTemplateType>()(std::declval<size_t>()))>
		FixedVector(size_t size, GeneratorTemplateType&& generator)
			: _max_size(size),
			_size(size),
			_allocator(),
			_data_start_ptr(allocator_traits::allocate(_allocator, size)) {
			for (size_t i = 0; i < size; ++i) {
				allocator_traits::construct(
					_allocator,
					begin()+i,
					generator(i)
				);
			}
		}

		//Generator (size_t i) -> std::tuple<Args...> (where T is constructable from Args)
		template<typename GeneratorTemplateType>
		// The generator must return a tuple
		requires specialization_of<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_t>>, std::tuple>
		// The tuple must be constructible into a T
		&& requires(GeneratorTemplateType&& generator) {
			{
				std::apply(
					[](auto&&... args) {
						T obj{std::forward<decltype(args)>(args)...};
					},
					generator(std::declval<size_t>())
				)
			};
		}
		FixedVector(size_t size, GeneratorTemplateType&& generator)
			: _max_size(size),
			_size(size),
			_allocator(),
			_data_start_ptr(allocator_traits::allocate(_allocator, size)) {
			for (size_t i = 0; i < size; ++i) {
				std::apply(
					[this, i](auto&&... args) {
						allocator_traits::construct(
							_allocator,
							begin()+i,
							std::forward<decltype(args)>(args)...
						);
					},
					generator(i)
				);
			}
		}

		FixedVector(const FixedVector&) = delete;
		FixedVector& operator=(const FixedVector&) = delete;
		FixedVector(FixedVector&&) = delete;
		FixedVector& operator=(FixedVector&&) = delete;

		~FixedVector() {
			clear();
			allocator_traits::deallocate(_allocator, _data_start_ptr, _max_size);
		}
		
		using iterator = T*;
		using const_iterator = const T*;

		iterator begin() { return _data_start_ptr; }
		const_iterator begin() const { return _data_start_ptr; }
		const_iterator cbegin() const { return _data_start_ptr; }

		iterator end() { return begin() + _size; }
		const_iterator end() const { return begin() + _size; }
		const_iterator cend() const { return cbegin() + _size; }
		
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		reverse_iterator rbegin() { return reverse_iterator(end()); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

		reverse_iterator rend() { return reverse_iterator(begin()); }
		const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

		T& operator[](size_t index) {
			assert(index < _size && "Index out of bounds.");
			return _data_start_ptr[index];
		}
		const T& operator[](size_t index) const {
			assert(index < _size && "Index out of bounds.");
			return _data_start_ptr[index];
		}

		T& front() {
			assert(!empty());
			return *begin();
		}
		const T& front() const {
			assert(!empty());
			return *cbegin();
		}
		T& back() {
			assert(!empty());
			return *(end()-1);
		}
		const T& back() const {
			assert(!empty());
			return *(cend()-1);
		}

		template <typename... Args>
		iterator emplace_back(Args&&... args) {
			if (_size >= _max_size) {
				return end();
			}
			allocator_traits::construct(
				_allocator,
				begin() + _size,
				std::forward<Args>(args)...
			);
			++_size;
			return end()-1;
		}

		void pop_back() {
    		if (_size > 0) {
				allocator_traits::destroy(
					_allocator,
					end() - 1
				);
				--_size;
			}
		}

		void clear() {
			for (iterator it = end(); it != begin(); ) {
				--it;
				allocator_traits::destroy(
					_allocator,
					it
				);
			}
			_size = 0;
		}
	};
}

#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/utility/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using FixedVector = _detail::FixedVector<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}