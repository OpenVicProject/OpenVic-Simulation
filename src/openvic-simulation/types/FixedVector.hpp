#pragma once

#include <cassert>
#include <memory>
#include <utility>

#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic::_detail {
	//fixed capacity + not movable + not copyable
	template <typename T, typename Allocator = std::allocator<T>>
	class FixedVector {
	private:
		using allocator_traits = std::allocator_traits<Allocator>;
		const size_t fixed_capacity;
		size_t current_size;
		OV_NO_UNIQUE_ADDRESS Allocator allocator;
		T* const data_start_ptr;

	public:
		constexpr size_t size() const { return current_size; }
		constexpr size_t capacity() const { return fixed_capacity; }

		explicit FixedVector(const size_t capacity)
			: fixed_capacity(capacity),
			current_size(0),
			allocator(),
			data_start_ptr(allocator_traits::allocate(allocator, capacity)) {}

		//Generator (size_t i) -> U (where T is constructable from U)
		template<typename GeneratorTemplateType>
		// The generator must NOT return a tuple
		requires (!utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_t>>, std::tuple>)
		// The type must be constructible from the generator's single return value
		&& std::constructible_from<T, decltype(std::declval<GeneratorTemplateType>()(std::declval<size_t>()))>
		FixedVector(size_t capacity, GeneratorTemplateType&& generator)
			: fixed_capacity(capacity),
			current_size(capacity),
			allocator(),
			data_start_ptr(allocator_traits::allocate(allocator, capacity)) {
			for (size_t i = 0; i < capacity; ++i) {
				allocator_traits::construct(
					allocator,
					begin()+i,
					generator(i)
				);
			}
		}

		//Generator (size_t i) -> std::tuple<Args...> (where T is constructable from Args)
		template<typename GeneratorTemplateType>
		// The generator must return a tuple
		requires utility::is_specialization_of_v<std::remove_cvref_t<std::invoke_result_t<GeneratorTemplateType, size_t>>, std::tuple>
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
		FixedVector(size_t capacity, GeneratorTemplateType&& generator)
			: fixed_capacity(capacity),
			current_size(capacity),
			allocator(),
			data_start_ptr(allocator_traits::allocate(allocator, capacity)) {
			for (size_t i = 0; i < capacity; ++i) {
				std::apply(
					[this, i](auto&&... args) {
						allocator_traits::construct(
							allocator,
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
			allocator_traits::deallocate(allocator, data_start_ptr, fixed_capacity);
		}
		
		using iterator = T*;
		using const_iterator = const T*;

		iterator begin() { return data_start_ptr; }
		const_iterator begin() const { return data_start_ptr; }
		const_iterator cbegin() const { return data_start_ptr; }

		iterator end() { return begin() + current_size; }
		const_iterator end() const { return begin() + current_size; }
		const_iterator cend() const { return cbegin() + current_size; }

		T& operator[](size_t index) {
			assert(index < current_size && "Index out of bounds.");
			return data_start_ptr[index];
		}
		const T& operator[](size_t index) const {
			assert(index < current_size && "Index out of bounds.");
			return data_start_ptr[index];
		}

		T& front() {
			assert(current_size > 0 && "Container is empty.");
			return *begin();
		}
		const T& front() const {
			assert(current_size > 0 && "Container is empty.");
			return *cbegin();
		}
		T& back() {
			assert(current_size > 0 && "Container is empty.");
			return *(end()-1);
		}
		const T& back() const {
			assert(current_size > 0 && "Container is empty.");
			return *(cend()-1);
		}

		template <typename... Args>
		iterator emplace_back(Args&&... args) {
			if (current_size >= fixed_capacity) {
				return end();
			}
			allocator_traits::construct(
				allocator,
				begin() + current_size,
				std::forward<Args>(args)...
			);
			++current_size;
			return end()-1;
		}

		void pop_back() {
    		if (current_size > 0) {
				allocator_traits::destroy(
					allocator,
					end() - 1
				);
				--current_size;
			}
		}

		void clear() {
			for (iterator it = end(); it != begin(); ) {
				--it;
				allocator_traits::destroy(
					allocator,
					it
				);
			}
			current_size = 0;
		}
	};
}

#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/utility/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using FixedVector = _detail::FixedVector<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}