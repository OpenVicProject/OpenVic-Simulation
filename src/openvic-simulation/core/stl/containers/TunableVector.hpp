#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include <range/v3/algorithm/move.hpp>

#include "openvic-simulation/core/Assert.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

namespace OpenVic::stl {
	template<size_t Num, size_t Denom, size_t InitAllocSize = 0>
	struct growth_traits {
		static constexpr std::integral_constant<size_t, Num> numerator {};
		static constexpr std::integral_constant<size_t, Denom> denominator {};
		static constexpr std::integral_constant<size_t, InitAllocSize> initial_allocation_size {};
	};

	// Uses approximated golden ratio (1.617...)
	using default_growth_traits = growth_traits<55, 34>;

	namespace _detail {
		template<typename T>
		concept tunable_growth_trait = requires() {
			{ T::numerator() } -> std::convertible_to<size_t>;
			{ T::denominator() } -> std::convertible_to<size_t>;
			{ T::initial_allocation_size() } -> std::convertible_to<size_t>;
		};

		template<typename Range, typename T>
		concept _compatible_range =
			std::ranges::input_range<Range> && std::convertible_to<std::ranges::range_reference_t<Range>, T>;
	}

	template<
		typename T, _detail::tunable_growth_trait GrowthTrait = default_growth_traits, typename Allocator = std::allocator<T>>
	class tunable_vector {
	public:
		using value_type = T;
		using allocator_type = Allocator;
		using container_type = std::vector<value_type, allocator_type>;
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
		using growth_trait = GrowthTrait;

		static_assert(growth_trait::denominator() != 0, "growth_trait's denominator cannot be 0");
		static_assert(growth_trait::numerator() > growth_trait::denominator(), "growth_trait's divisor must be more than 1");

		constexpr tunable_vector() = default;
		explicit constexpr tunable_vector(allocator_type const& alloc) : _container(alloc) {}
		explicit constexpr tunable_vector(size_type count, allocator_type const& alloc = allocator_type {})
			: _container(count, alloc) {}
		constexpr tunable_vector(size_type count, value_type const& value, allocator_type const& alloc = allocator_type {})
			: _container(count, value, alloc) {}
		template<typename InputIt>
		constexpr tunable_vector(InputIt first, InputIt last, allocator_type const& alloc = allocator_type {})
			: _container(first, last, alloc) {}

		constexpr tunable_vector(container_type const& other) : _container { other } {}
		constexpr tunable_vector(container_type&& other) : _container { std::move(other) } {}

		constexpr tunable_vector(container_type const& other, allocator_type const& alloc) : _container { other, alloc } {}
		constexpr tunable_vector(container_type&& other, allocator_type const& alloc)
			: _container { std::move(other), alloc } {}

		constexpr tunable_vector(tunable_vector const& other) : _container { other._container } {}
		constexpr tunable_vector(tunable_vector&& other) : _container { std::move(other._container) } {}

		constexpr tunable_vector(tunable_vector const& other, allocator_type const& alloc)
			: _container { other._container, alloc } {}
		constexpr tunable_vector(tunable_vector&& other, allocator_type const& alloc)
			: _container { std::move(other._container), alloc } {}

		template<typename OtherGrowth>
		constexpr tunable_vector(tunable_vector<T, OtherGrowth, Allocator> const& other) : _container { other._container } {}
		template<typename OtherGrowth>
		constexpr tunable_vector(tunable_vector<T, OtherGrowth, Allocator>&& other)
			: _container { std::move(other._container) } {}

		template<typename OtherGrowth>
		constexpr tunable_vector(tunable_vector<T, OtherGrowth, Allocator> const& other, allocator_type const& alloc)
			: _container { other._container, alloc } {}
		template<typename OtherGrowth>
		constexpr tunable_vector(tunable_vector<T, OtherGrowth, Allocator>&& other, allocator_type const& alloc)
			: _container { std::move(other._container), alloc } {}

		constexpr tunable_vector(std::initializer_list<value_type> init, allocator_type const& alloc = allocator_type {})
			: _container(init, alloc) {}

		constexpr tunable_vector& operator=(container_type const& other) {
			_container = other;
			return *this;
		}
		constexpr tunable_vector& operator=(container_type&& other) {
			_container = std::move(other);
			return *this;
		}

		constexpr tunable_vector& operator=(tunable_vector const& other) {
			_container = other._container;
			return *this;
		}
		constexpr tunable_vector& operator=(tunable_vector&& other) {
			_container = std::move(other._container);
			return *this;
		}

		tunable_vector& operator=(std::initializer_list<value_type> ilist) {
			_container = ilist;
			return *this;
		}

		[[nodiscard]] constexpr reference operator[](const size_type pos) {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return _container[pos];
		}

		[[nodiscard]] constexpr const_reference operator[](const size_type pos) const {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return _container[pos];
		}

		[[nodiscard]] constexpr reference front() {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return _container[0];
		}

		[[nodiscard]] constexpr const_reference front() const {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return _container[0];
		}

		[[nodiscard]] constexpr reference back() {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _container[size() - 1];
		}

		[[nodiscard]] constexpr const_reference back() const {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _container[size() - 1];
		}

		[[nodiscard]] constexpr value_type* data() {
			return _container.data();
		}

		[[nodiscard]] constexpr value_type const* data() const {
			return _container.data();
		}

		[[nodiscard]] constexpr iterator begin() {
			return _container.begin();
		}

		[[nodiscard]] constexpr const_iterator begin() const {
			return _container.begin();
		}

		[[nodiscard]] constexpr const_iterator cbegin() const {
			return _container.cbegin();
		}

		[[nodiscard]] constexpr iterator end() {
			return _container.end();
		}

		[[nodiscard]] constexpr const_iterator end() const {
			return _container.end();
		}

		[[nodiscard]] constexpr const_iterator cend() const {
			return _container.cend();
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() {
			return _container.rbegin();
		}

		[[nodiscard]] constexpr const_reverse_iterator rbegin() const {
			return _container.rbegin();
		}

		[[nodiscard]] constexpr const_reverse_iterator crbegin() const {
			return _container.crbegin();
		}

		[[nodiscard]] constexpr reverse_iterator rend() {
			return _container.rend();
		}

		[[nodiscard]] constexpr const_reverse_iterator rend() const {
			return _container.rend();
		}

		constexpr const_reverse_iterator crend() const {
			return _container.crend();
		}

		[[nodiscard]] constexpr bool empty() const {
			return _container.empty();
		}

		[[nodiscard]] constexpr size_type size() const {
			return _container.size();
		}

		[[nodiscard]] constexpr size_type max_size() const {
			return _container.max_size();
		}

		[[nodiscard]] constexpr size_type capacity() const {
			return _container.capacity();
		}

		constexpr void shrink_to_fit() {
			_container.shrink_to_fit();
		}

		constexpr void clear() {
			_container.clear();
		}

		constexpr iterator erase(const_iterator pos) {
			return _container.erase(pos);
		}

		constexpr iterator erase(const_iterator first, const_iterator last) {
			return _container.erase(first, last);
		}

		constexpr void swap(tunable_vector& vector) {
			_container.swap(vector._container);
		}

		constexpr allocator_type get_allocator() const {
			return _container.get_allocator();
		}

		constexpr void push_back(value_type const& value) {
			if (size() == capacity()) {
				reserve_minimum(capacity() + 1);
			}
			_container.push_back(value);
		}

		constexpr void push_back(value_type&& value) {
			if (size() == capacity()) {
				reserve_minimum(capacity() + 1);
			}
			_container.push_back(std::move(value));
		}

		template<typename... Args>
		constexpr reference emplace_back(Args&&... args) {
			if (size() == capacity()) {
				reserve_minimum(capacity() + 1);
			}
			return _container.emplace_back(std::forward<Args>(args)...);
		}

		template<_detail::_compatible_range<T> RangeT>
		constexpr void append_range(RangeT&& range) {
			if constexpr (std::ranges::forward_range<RangeT> || std::ranges::sized_range<RangeT>) {
				reserve_minimum(std::ranges::distance(range));
				ranges::move(range, std::back_inserter(*this));
			} else {
				auto first = std::ranges::begin(range);
				const auto last = std::ranges::end(range);

				for (size_type free = capacity() - size(); first != last && free != size_type {};
					 std::ranges::advance(first, 1), --free) {
					emplace_back(*first);
				}

				if (first == last) {
					return;
				}

				tunable_vector<value_type> tmp { get_allocator() };
				for (; first != last; std::ranges::advance(first, 1)) {
					tmp.emplace_back(*first);
				}
				std::ranges::subrange subrange(std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()));
				append_range(subrange);
			}
		}

		constexpr void resize(size_type count) {
			reserve_minimum(count);
			_container.resize(count);
		}

		// Prefer reserve_minimum to be more explicit and obvious.
		// This is only for making it a drop-in replacement of vector.
		constexpr void reserve(size_type count) {
			reserve_minimum(count);
		}

		constexpr void reserve_minimum(size_type count) {
			if (count > capacity()) {
				_container.reserve(_get_capacity_for(count));
			}
		}

		constexpr void reserve_exact(size_type count) {
			_container.reserve(count);
		}

		[[nodiscard]] constexpr container_type const& container() const {
			return _container;
		}

		constexpr container_type&& release() && {
			return std::move(_container);
		}

	private:
		container_type _container;

		[[nodiscard]] constexpr size_type _get_capacity_for(const size_type value) const {
			const size_type max = max_size();
			const size_type old_capacity = capacity();

			if (max <= value) {
				return max;
			}

			if (value <= old_capacity) {
				return value;
			}

			const size_type new_capacity = std::max<size_type>(1, old_capacity * growth_trait::numerator() / growth_trait::denominator());

			// If true, new_capacity overflowed something
			if (OV_unlikely(new_capacity == size_type {} || new_capacity < old_capacity || new_capacity > max)) {
				return max;
			}

			if (new_capacity < growth_trait::initial_allocation_size()) {
				return growth_trait::initial_allocation_size();
			}

			// Something odd has happened
			if (OV_unlikely(new_capacity / growth_trait::numerator() < old_capacity / growth_trait::denominator())) {
				return value;
			}

			if (value > new_capacity) {
				return value;
			}

			return new_capacity;
		}
	};
}
