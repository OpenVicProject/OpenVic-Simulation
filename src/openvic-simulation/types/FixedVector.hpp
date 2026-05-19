#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include "openvic-simulation/core/Assert.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"

namespace OpenVic::_detail {
	// fixed capacity vector
	// supports immovable + uncopyable value types
	template <typename T, typename SizeT = std::size_t, typename Allocator = std::allocator<T>>
	class FixedVector {
	public:
		using const_reference = T const&;
		using difference_type = ptrdiff_t;
		using reference = T&;
		using size_type = SizeT;
		using value_type = T;
	private:
		using allocator_traits = std::allocator_traits<Allocator>;
		size_type _max_size;
		size_type _size;
		OV_NO_UNIQUE_ADDRESS Allocator _allocator;
		T* _data_start_ptr;

		static constexpr T* allocate(Allocator allocator, const size_type capacity) {
			return capacity > size_type{}
				? allocator_traits::allocate(allocator, get_index_as_size_t(capacity))
				: nullptr;
		}

	public:
		constexpr size_type size() const { return _size; }
		constexpr size_type capacity() const { return _max_size; }
		constexpr size_type max_size() const { return _max_size; }
		constexpr T* data() { return _data_start_ptr; }
		constexpr T const* data() const { return _data_start_ptr; }
		constexpr bool empty() const { return _size == size_type{}; }

		constexpr explicit FixedVector(const create_empty_t t) noexcept : FixedVector(t, size_type{}) {}
		/**
		* @brief Creates an uninitialised vector with fixed capacity
		*/
		constexpr explicit FixedVector(const create_empty_t, const size_type capacity)
			: _max_size(capacity),
			_size(size_type{}),
			_allocator(),
			_data_start_ptr(allocate(_allocator, capacity)) {}

		constexpr FixedVector(const size_type size, T const& value_for_all_indices)
			: _max_size(size),
			_size(size),
			_allocator(),
			_data_start_ptr(allocate(_allocator, size)) {
			std::fill(_data_start_ptr, _data_start_ptr + get_index_as_size_t(size), value_for_all_indices);
		}
		
		constexpr explicit FixedVector(const generate_values_t, const size_type capacity)
		requires (
			std::constructible_from<value_type, size_type>
			|| std::is_default_constructible_v<value_type>
		) : FixedVector(
			capacity,
			[](const size_type i)->value_type {
				if constexpr (std::constructible_from<value_type, size_type>) {
					return value_type{i};
				} else {
					return value_type{};
				}
			}
		) {}

		//Generator (size_type i) -> U (where T is constructable from U)
		template<typename GeneratorTemplateType>
		// The generator must NOT return a tuple
		requires (!specialization_of<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_type>>, std::tuple>)
		// The type must be constructible from the generator's single return value
		&& std::constructible_from<T, decltype(std::declval<GeneratorTemplateType>()(std::declval<size_type>()))>
		constexpr FixedVector(const size_type size, GeneratorTemplateType&& generator)
			: _max_size(size),
			_size(size_type{}),
			_allocator(),
			_data_start_ptr(allocate(_allocator, size)) {
			for (size_type i {}; i < size; ++i) {
				allocator_traits::construct(
					_allocator,
					begin()+get_index_as_size_t(i),
					generator(i)
				);
				++_size;
			}
		}

		//Generator (size_type i) -> std::tuple<Args...> (where T is constructable from Args)
		template<typename GeneratorTemplateType>
		// The generator must return a tuple
		requires specialization_of<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_type>>, std::tuple>
		// The tuple must be constructible into a T
		&& requires(GeneratorTemplateType&& generator) {
			{
				std::apply(
					[](auto&&... args) {
						T obj{std::forward<decltype(args)>(args)...};
					},
					generator(std::declval<size_type>())
				)
			};
		}
		constexpr FixedVector(const size_type size, GeneratorTemplateType&& generator)
			: _max_size(size),
			_size(size_type{}),
			_allocator(),
			_data_start_ptr(allocate(_allocator, size)) {
			for (size_type i {}; i < size; ++i) {
				std::apply(
					[this, i](auto&&... args) {
						allocator_traits::construct(
							_allocator,
							begin()+get_index_as_size_t(i),
							std::forward<decltype(args)>(args)...
						);
					},
					generator(i)
				);
				++_size;
			}
		}

		FixedVector(const FixedVector&) = delete;
		FixedVector& operator=(const FixedVector&) = delete;

		constexpr FixedVector(FixedVector&& other) noexcept : FixedVector(create_empty) {
			swap(other);
		}
		constexpr FixedVector& operator=(FixedVector&& other) noexcept {
			swap(other);
			return *this;
		}
		constexpr void swap(FixedVector& other) noexcept {
			std::swap(_max_size, other._max_size);
			std::swap(_size, other._size);
			std::swap(_allocator, other._allocator);
			std::swap(_data_start_ptr, other._data_start_ptr);
		}
		constexpr ~FixedVector() {
			if (!_data_start_ptr) {
				return;
			}
			clear();
			allocator_traits::deallocate(_allocator, _data_start_ptr, get_index_as_size_t(_max_size));
		}
		
		using iterator = T*;
		using const_iterator = T const*;

		constexpr iterator begin() { return _data_start_ptr; }
		constexpr const_iterator begin() const { return _data_start_ptr; }
		constexpr const_iterator cbegin() const { return _data_start_ptr; }

		constexpr iterator end() { return begin() + get_index_as_size_t(_size); }
		constexpr const_iterator end() const { return begin() + get_index_as_size_t(_size); }
		constexpr const_iterator cend() const { return cbegin() + get_index_as_size_t(_size); }
		
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }
		constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
		constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

		constexpr reverse_iterator rend() { return reverse_iterator(begin()); }
		constexpr const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
		constexpr const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

		constexpr T& operator[](const size_type typed_index) {
			OV_HARDEN_ASSERT_ACCESS(typed_index, "operator[]");
			return _data_start_ptr[get_index_as_size_t(typed_index)];
		}
		constexpr T const& operator[](const size_type typed_index) const {
			OV_HARDEN_ASSERT_ACCESS(typed_index, "operator[]");
			return _data_start_ptr[get_index_as_size_t(typed_index)];
		}

		constexpr T& front() {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return *begin();
		}
		constexpr T const& front() const {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return *cbegin();
		}
		constexpr T& back() {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return *(end()-1);
		}
		constexpr T const& back() const {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return *(cend()-1);
		}

		template <typename... Args>
		constexpr iterator emplace_back(Args&&... args) {
			if (_size >= _max_size) {
				return end();
			}
			allocator_traits::construct(
				_allocator,
				begin() + get_index_as_size_t(_size),
				std::forward<Args>(args)...
			);
			++_size;
			return end()-1;
		}

		constexpr void pop_back() {
    		if (_size > size_type{}) {
				allocator_traits::destroy(
					_allocator,
					end() - 1
				);
				--_size;
			}
		}

		constexpr void clear() {
			for (iterator it = end(); it != begin(); ) {
				--it;
				allocator_traits::destroy(
					_allocator,
					it
				);
			}
			_size = {};
		}
	};
}

#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename T, typename SizeT = std::size_t, class RawAllocator = foonathan::memory::default_allocator>
	using FixedVector = _detail::FixedVector<T, SizeT, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}