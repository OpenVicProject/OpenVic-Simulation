#pragma once

#include <atomic>
#include <cassert>
#include <iterator>
#include <type_traits>
#include <utility>

#include <range/v3/algorithm/copy.hpp>

#include "openvic-simulation/core/Assert.hpp"

namespace OpenVic {
	// not thread safe
	template <typename Container>
	struct bulk_insert_wrapper {
	public:
		// Member types based on std::vector
		using container_type = Container;
		using value_type = typename container_type::value_type;
		using allocator_type = typename container_type::allocator_type;
		using size_type = typename container_type::size_type;
		using difference_type = typename container_type::difference_type;
		using reference = typename container_type::reference;
		using const_reference = typename container_type::const_reference;
		using pointer = typename container_type::pointer;
		using const_pointer = typename container_type::const_pointer;
		using iterator = typename container_type::iterator;
		using const_iterator = typename container_type::const_iterator;
		using reverse_iterator = typename container_type::reverse_iterator;
		using const_reverse_iterator = typename container_type::const_reverse_iterator;

		static_assert(std::is_default_constructible_v<value_type>);
		static_assert(std::is_trivially_destructible_v<value_type>);

	private:
		Container container;
		size_type valid_size {};
		std::atomic<size_type> pending_extra_size {};

		constexpr void flush_pending_room() {
			if (pending_extra_size > size_type{}) {
				container.resize(valid_size + pending_extra_size);
				pending_extra_size = size_type{};
			}
		}

		constexpr size_type get_invalid_element_count() const {
			return container.size() - valid_size;
		}
		constexpr bool has_invalid_element() const {
			return container.size() > valid_size;
		}

	public:
		constexpr allocator_type get_allocator() const {
			return container.get_allocator();
		}

		constexpr bulk_insert_wrapper() noexcept {};

		// Forwarding constructor for custom allocators or initial capacities
		template <typename... Args>
		constexpr explicit bulk_insert_wrapper(Args&&... args)
			: container(std::forward<Args>(args)...),
			  valid_size(container.size()) {}

		// thread safe
		constexpr void make_room_for(const size_type count) noexcept {
			pending_extra_size += count;
		}

		// Element access based on std::vector
		constexpr reference operator[](const size_type pos) {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return container[pos];
		}
		constexpr const_reference operator[](const size_type pos) const {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return container[pos];
		}

		constexpr reference front() {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return container.front();
		}
		constexpr const_reference front() const {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return container.front();
		}
		
		constexpr reference back() {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return container[size()-1];
		}
		constexpr const_reference back() const {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return container[size()-1];
		}

		constexpr value_type* data() noexcept { return container.data(); }
		constexpr value_type const* data() const noexcept { return container.data(); }

		// Iterators based on std::vector
		constexpr iterator begin() noexcept {
			return container.begin();
		}
		constexpr const_iterator begin() const noexcept {
			return container.begin();
		}
		constexpr const_iterator cbegin() const noexcept {
			return container.cbegin();
		}

		constexpr iterator end() noexcept {
			return std::next(begin(), valid_size);
		}
		constexpr const_iterator end() const noexcept {
			return std::next(begin(), valid_size);
		}
		constexpr const_iterator cend() const noexcept {
			return std::next(cbegin(), valid_size);
		}

		constexpr reverse_iterator rbegin() noexcept {
			return std::make_reverse_iterator(end());
		}
		constexpr const_reverse_iterator rbegin() const noexcept {
			return std::make_reverse_iterator(end());
		}
		constexpr const_reverse_iterator crbegin() const noexcept {
			return std::make_reverse_iterator(cend());
		}

		constexpr reverse_iterator rend() noexcept {
			return container.rend();
		}
		constexpr const_reverse_iterator rend() const noexcept {
			return container.rend();
		}
		constexpr const_reverse_iterator crend() const noexcept {
			return container.crend();
		}

		// Capacity based on std::vector
		constexpr bool empty() const noexcept { return valid_size <= size_type{}; }
		constexpr size_type size() const noexcept { return valid_size; }
		constexpr size_type max_size() const noexcept { return container.max_size(); }
		// reserve() is omitted as we manage that via make_room_for
		constexpr size_type capacity() const noexcept { return container.capacity(); }
		constexpr void shrink_to_fit() {
			pending_extra_size = size_type{};
			container.resize(valid_size);
			container.shrink_to_fit();
		}

		// Modifiers based on std::vector
		constexpr void clear() noexcept {
			valid_size = size_type{};
			pending_extra_size = size_type{};
			container.clear();
		}

		// the following could be implemented:
		// - insert
		// - insert_range
		// - emplace
		// - erase
		// - append_range (C++23)
		// - pop_back
		// - swap

		constexpr void push_back(value_type const& value) {
			flush_pending_room();
			if (has_invalid_element()) {
				container[valid_size] = value;
			} else {
				container.push_back(value);
			}
			++valid_size;

		}
		constexpr void push_back(value_type&& value) {
			flush_pending_room();
			if (has_invalid_element()) {
				container[valid_size] = std::move(value);
			} else {
				container.push_back(std::move(value));
			}
			++valid_size;
		}

		template<typename... Args>
		requires std::is_trivially_destructible_v<value_type>
		constexpr reference emplace_back(Args&&... args) {
			const size_type old_total_size = container.size();
			if (old_total_size > valid_size) {
				// ensure container.emplace_back places it after the last valid element
				container.resize(valid_size);
			}

			reference result = container.emplace_back(std::forward<Args>(args)...);
			++valid_size;

			if (old_total_size > valid_size) {
				// restore extra default constructed elements so they can be overwritten in append_range
				// since we're resizing anyway at this point, might as well resize for pending_extra_size
				const size_type desired_size = std::max(
					old_total_size,
					valid_size + pending_extra_size
				);
				container.resize(desired_size);
				pending_extra_size = size_type{};
			}

			return result;
		}

		template <typename OtherContainerT>
		constexpr void append_range(OtherContainerT const& other) {
			append_range(other.begin(), other.end());
		}

		template <typename InputIt>
		constexpr void append_range(const InputIt first, const InputIt last) {
			flush_pending_room();
			
			const size_type new_valid_size = valid_size + std::distance(first, last);
			if (new_valid_size > container.size()) {
				assert(!"append_range called without make_room_for");
				container.resize(new_valid_size);
			}

			ranges::copy(
				first,
				last,
				end()
			);
			valid_size = new_valid_size;
		}

		// resize() is omitted as we manage that via make_room_for
	};
}