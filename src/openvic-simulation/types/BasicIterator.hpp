#pragma once

#include <concepts>
#include <iterator>

#include "openvic-simulation/utility/Typedefs.hpp"
#include "openvic-simulation/utility/Compare.hpp"

namespace OpenVic {
	template<typename Pointer, typename ContainerTag>
	struct basic_iterator {
		using iterator_type = Pointer;
		using value_type = typename std::iterator_traits<iterator_type>::value_type;
		using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
		using pointer = typename std::iterator_traits<iterator_type>::pointer;
		using reference = typename std::iterator_traits<iterator_type>::reference;
		using iterator_category = typename std::iterator_traits<iterator_type>::iterator_category;
		using iterator_concept = std::contiguous_iterator_tag;

		OV_ALWAYS_INLINE constexpr basic_iterator() {};
		OV_ALWAYS_INLINE constexpr basic_iterator(Pointer const& ptr) : _current { ptr } {}

		template<std::convertible_to<Pointer> It>
		OV_ALWAYS_INLINE constexpr basic_iterator(basic_iterator<It, ContainerTag> const& i) : _current(i.base()) {}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr reference operator*() const {
			return *_current;
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr pointer operator->() const {
			return _current;
		}

		OV_ALWAYS_INLINE constexpr basic_iterator& operator++() {
			++_current;
			return *this;
		}

		OV_ALWAYS_INLINE constexpr basic_iterator operator++(int) {
			return basic_iterator(_current++);
		}

		OV_ALWAYS_INLINE constexpr basic_iterator& operator--() {
			--_current;
			return *this;
		}

		OV_ALWAYS_INLINE constexpr basic_iterator operator--(int) {
			return basic_iterator(_current--);
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr reference operator[](difference_type index) const {
			return _current[index];
		}

		OV_ALWAYS_INLINE constexpr basic_iterator& operator+=(difference_type index) {
			_current += index;
			return *this;
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr basic_iterator operator+(difference_type index) const {
			return basic_iterator(_current + index);
		}

		OV_ALWAYS_INLINE constexpr basic_iterator& operator-=(difference_type index) {
			_current -= index;
			return *this;
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr basic_iterator operator-(difference_type index) const {
			return basic_iterator(_current - index);
		}

		[[nodiscard]] OV_ALWAYS_INLINE constexpr iterator_type const& base() const {
			return _current;
		}

	protected:
		iterator_type _current {};
	};

	template<typename PtrL, typename PtrR, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr bool operator==( //
		basic_iterator<PtrL, ContainerTag> const& lhs, basic_iterator<PtrR, ContainerTag> const& rhs
	)
	requires requires {
		{ lhs.base() == rhs.base() } -> std::convertible_to<bool>;
	}
	{
		return lhs.base() == rhs.base();
	}

	template<typename PtrL, typename PtrR, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr auto operator<=>( //
		basic_iterator<PtrL, ContainerTag> const& lhs, basic_iterator<PtrR, ContainerTag> const& rhs
	) {
		return three_way_compare(lhs.base(), rhs.base());
	}

	template<typename Ptr, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr bool operator==( //
		basic_iterator<Ptr, ContainerTag> const& lhs, basic_iterator<Ptr, ContainerTag> const& rhs
	)
	requires requires {
		{ lhs.base() == rhs.base() } -> std::convertible_to<bool>;
	}
	{
		return lhs.base() == rhs.base();
	}

	template<typename Ptr, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr auto operator<=>( //
		basic_iterator<Ptr, ContainerTag> const& lhs, basic_iterator<Ptr, ContainerTag> const& rhs
	) {
		return three_way_compare(lhs.base(), rhs.base());
	}

	template<typename ItL, typename ItR, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr auto operator-( //
		basic_iterator<ItL, ContainerTag> const& lhs, basic_iterator<ItR, ContainerTag> const& rhs
	) -> decltype(lhs.base() - rhs.base()) {
		return lhs.base() - rhs.base();
	}

	template<typename It, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr typename basic_iterator<It, ContainerTag>::difference_type operator-( //
		basic_iterator<It, ContainerTag> const& lhs, basic_iterator<It, ContainerTag> const& rhs
	) {
		return lhs.base() - rhs.base();
	}

	template<typename It, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr basic_iterator<It, ContainerTag> operator+( //
		typename basic_iterator<It, ContainerTag>::difference_type n, basic_iterator<It, ContainerTag> const& i
	) {
		return basic_iterator<It, ContainerTag>(i.base() + n);
	}

	template<typename It, typename ContainerTag>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr It iterator_base(basic_iterator<It, ContainerTag> it) {
		return it.base();
	}

	template<typename It>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr It iterator_base(It it) {
		return it;
	}

	template<typename It>
	constexpr inline auto iterator_base(std::reverse_iterator<It> it)
		-> decltype(std::make_reverse_iterator(iterator_base(it.base()))) {
		return std::make_reverse_iterator(iterator_base(it.base()));
	}

	template<typename It>
	constexpr inline auto iterator_base(std::move_iterator<It> it)
		-> decltype(std::make_move_iterator(iterator_base(it.base()))) {
		return std::make_move_iterator(iterator_base(it.base()));
	}

	template<typename From, typename To>
	[[nodiscard]] constexpr inline From unwrap_iterator(From from, To res) {
		return from + (iterator_base(res) - iterator_base(from));
	}

	template<typename It>
	[[nodiscard]] OV_ALWAYS_INLINE constexpr It unwrap_iterator(It const&, It res) {
		return res;
	}
};
