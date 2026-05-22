#pragma once

#include <type_traits>
#include <utility>

#include <foonathan/memory/default_allocator.hpp>

#include "openvic-simulation/core/Assert.hpp"
#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"

namespace OpenVic {
	struct owning_registry_internal_tag {};

	template<
		typename ValueType,
		typename SizeType,
		typename Allocator = foonathan::memory::default_allocator
	>
	struct OwningRegistry {
	private:
		bool _is_locked { false };
		memory::FixedVector<ValueType, SizeType> _values;
		
		constexpr void swap(OwningRegistry& other) noexcept {
			std::swap(_is_locked, other._is_locked);
			std::swap(_values, other._values);
		}

	public:
		constexpr OwningRegistry(OwningRegistry&& other) noexcept : _values(create_empty) {
			swap(other);
		}

		template<typename... Args>
		requires (sizeof...(Args) != 1 || (!std::is_same_v<std::decay_t<Args>, OwningRegistry> && ...))
		constexpr OwningRegistry(Args&&... args) : _values(std::forward<Args>(args)...) {}

		[[nodiscard]] constexpr bool is_locked() const { return _is_locked; }
		[[nodiscard]] constexpr bool& is_locked_internal(owning_registry_internal_tag) { return _is_locked; }
		[[nodiscard]] constexpr decltype(_values)& values_internal(owning_registry_internal_tag) { return _values; }
		constexpr void clear_and_reserve(const SizeType new_cap) noexcept {
			assert(!_is_locked);
			_is_locked = false;
			_values = std::move(
				decltype(_values) {
					create_empty,
					new_cap
				}
			);
		}

		// Member types based on std::vector
		using value_type = typename decltype(_values)::value_type;
		using size_type = SizeType;
		using difference_type = typename decltype(_values)::difference_type;
		using reference = typename decltype(_values)::reference;
		using const_reference = typename decltype(_values)::const_reference;
		using iterator = typename decltype(_values)::iterator;
		using const_iterator = typename decltype(_values)::const_iterator;
		using reverse_iterator = typename decltype(_values)::reverse_iterator;
		using const_reverse_iterator = typename decltype(_values)::const_reverse_iterator;

		// Element access based on std::vector
		[[nodiscard]] constexpr reference operator[](const size_type pos) {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return _values[pos];
		}
		[[nodiscard]] constexpr const_reference operator[](const size_type pos) const {
			OV_HARDEN_ASSERT_ACCESS(pos, "operator[]");
			return _values[pos];
		}

		[[nodiscard]] constexpr reference front() {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return _values[0];
		}
		[[nodiscard]] constexpr const_reference front() const {
			OV_HARDEN_ASSERT_NONEMPTY("front");
			return _values[0];
		}
		
		[[nodiscard]] constexpr reference back() {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _values[_values.size()-1];
		}
		[[nodiscard]] constexpr const_reference back() const {
			OV_HARDEN_ASSERT_NONEMPTY("back");
			return _values[_values.size()-1];
		}

		[[nodiscard]] constexpr value_type* data() noexcept { return _values.data(); }
		[[nodiscard]] constexpr value_type const* data() const noexcept { return _values.data(); }

		// Iterators based on std::vector
		[[nodiscard]] constexpr iterator begin() noexcept { return _values.begin(); }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return _values.begin(); }
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return _values.cbegin(); }

		[[nodiscard]] constexpr iterator end() noexcept { return _values.end(); }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return _values.end(); }
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return _values.cend(); }

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return _values.rbegin(); }
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return _values.rbegin(); }
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return _values.crbegin(); }

		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return _values.rend(); }
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return _values.rend(); }
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return _values.crend(); }

		// Capacity based on std::vector
		[[nodiscard]] constexpr bool empty() const noexcept { return _values.empty(); }
		[[nodiscard]] constexpr size_type size() const noexcept { return _values.size(); }
		[[nodiscard]] constexpr size_type max_size() const noexcept { return _values.max_size(); }
		[[nodiscard]] constexpr size_type capacity() const noexcept { return _values.capacity(); }

		// Modifiers omitted on purpose
	};
}