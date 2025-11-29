#pragma once

#if __has_include(<__fwd/span.h>)

#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <ranges>
#include <type_traits>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/types/BasicIterator.hpp"

namespace OpenVic::_detail::forwardable_span {
	static constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

	template<class T, std::size_t Extent = dynamic_extent>
	struct span;

	template<class T>
	struct is_std_span : std::false_type {};

	template<class T, std::size_t Extent>
	struct is_std_span<span<T, Extent>> : std::true_type {};

	template<class T, std::size_t Extent>
	struct is_std_span<std::span<T, Extent>> : std::true_type {};

	template<typename T>
	struct is_std_array : std::false_type {};

	template<typename T, std::size_t Size>
	struct is_std_array<std::array<T, Size>> : std::true_type {};

	template<class Range, class ElementType>
	concept span_compatible_range = !is_std_span<std::remove_cvref_t<Range>>::value && //
		std::ranges::contiguous_range<Range> && //
		std::ranges::sized_range<Range> && //
		(std::ranges::borrowed_range<Range> || std::is_const_v<ElementType>) && //
		!is_std_array<std::remove_cvref_t<Range>>::value && //
		!std::is_array_v<std::remove_cvref_t<Range>> && //
		std::is_convertible_v<std::remove_reference_t<std::ranges::range_reference_t<Range>> (*)[], ElementType (*)[]>;

	template<class From, class To>
	concept span_array_convertible = std::is_convertible_v<From (*)[], To (*)[]>;

	template<class It, class T>
	concept span_compatible_iterator =
		std::contiguous_iterator<It> && span_array_convertible<std::remove_reference_t<std::iter_reference_t<It>>, T>;

	template<class Sentinel, class It>
	concept span_compatible_sentinel_for =
		std::sized_sentinel_for<Sentinel, It> && !std::is_convertible_v<Sentinel, std::size_t>;

	template<size_t Extent>
	struct extent_storage {
		constexpr extent_storage([[maybe_unused]] std::size_t n) {
			if (std::is_constant_evaluated() && !(n == Extent)) {
				std::abort();
			}
		}

		consteval extent_storage(std::integral_constant<std::size_t, Extent>) {}

		template<size_t Gob>
		extent_storage(std::integral_constant<std::size_t, Gob>) = delete;

		OV_ALWAYS_INLINE static constexpr std::size_t extent() {
			return Extent;
		}
	};

	template<>
	struct extent_storage<dynamic_extent> {
		OV_ALWAYS_INLINE constexpr extent_storage(size_t extent) : _value(extent) {}

		OV_ALWAYS_INLINE constexpr std::size_t extent() const {
			return this->_value;
		}

	private:
		std::size_t _value;
	};

	template<typename T, std::size_t Extent>
	struct span {
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = T const*;
		using reference = T&;
		using const_reference = T const&;
		using iterator = basic_iterator<std::conditional_t<std::is_const_v<T>, const_pointer, pointer>, value_type*>;
		using reverse_iterator = std::reverse_iterator<iterator>;

	private:
		template<typename InT, size_t ArrayExtent>
		requires(Extent == dynamic_extent || ArrayExtent == Extent)
		struct _is_compatible_array : std::bool_constant<span_array_convertible<T, InT>> {};

		template<typename Ref>
		struct _is_compatible_ref : std::bool_constant<span_array_convertible<T, std::remove_reference_t<Ref>>> {};

		template<size_t Value>
		static inline constexpr std::integral_constant<std::size_t, Value> _v {};

		template<size_t Offset, size_t Count>
		static constexpr size_t subspan_extent() {
			if constexpr (Count != dynamic_extent) {
				return Count;
			} else if constexpr (extent != dynamic_extent) {
				return Extent - Offset;
			} else {
				return dynamic_extent;
			}
		}

	public:
		static constexpr size_type extent = Extent;

		constexpr span()
		requires(Extent == dynamic_extent || Extent == 0)
			: _ptr(nullptr), _extent(_v<0>) {}

		constexpr span(const span&) = default;

		template<span_compatible_iterator<element_type> It>
		constexpr explicit(extent != dynamic_extent) span(It first, size_type count)
			: _ptr(std::to_address(first)), _extent(count) {}

		template<span_compatible_iterator<element_type> It, span_compatible_sentinel_for<It> End>
		constexpr explicit(extent != dynamic_extent) span(It first, End last)
			: _ptr(std::to_address(first)), _extent(static_cast<size_type>(last - first)) {}

		template<std::size_t ArrayExtent>
		requires(Extent == dynamic_extent || ArrayExtent == Extent)
		constexpr span(std::type_identity_t<element_type> (&arr)[ArrayExtent]) : _ptr(arr), _extent(_v<ArrayExtent>) {}

		template<span_array_convertible<element_type> OtherElementType, std::size_t ArrayExtent>
		constexpr span(std::array<OtherElementType, ArrayExtent>& arr) : _ptr(arr.data()), _extent(_v<ArrayExtent>) {}

		template<typename OtherElementType, std::size_t ArrayExtent>
		requires span_array_convertible<const OtherElementType, element_type>
		constexpr span(std::array<OtherElementType, ArrayExtent> const& arr) : _ptr(arr.data()), _extent(_v<ArrayExtent>) {}

		template<span_compatible_range<element_type> Range>
		constexpr explicit(extent != dynamic_extent) span(Range&& r)
			: _ptr { std::ranges::data(r) }, _extent(std::ranges::size(r)) {}

		template<span_array_convertible<element_type> OtherElementType, std::size_t OtherExtent>
		requires(Extent == dynamic_extent || OtherExtent == dynamic_extent || Extent == OtherExtent)
		constexpr explicit(extent != dynamic_extent && OtherExtent == dynamic_extent)
			span(span<OtherElementType, OtherExtent> const& s)
			: _ptr(s.data()), _extent(s.size()) {}

		template<span_array_convertible<element_type> OtherElementType, std::size_t OtherExtent>
		requires(Extent == dynamic_extent || OtherExtent == dynamic_extent || Extent == OtherExtent)
		constexpr explicit(extent != dynamic_extent && OtherExtent == dynamic_extent)
			span(std::span<OtherElementType, OtherExtent> const& s)
			: _ptr(s.data()), _extent(s.size()) {}


		constexpr span& operator=(span const&) = default;

		[[nodiscard]] constexpr size_type size() const {
			return this->_extent.extent();
		}

		[[nodiscard]] constexpr size_type size_bytes() const {
			return this->_extent.extent() * sizeof(element_type);
		}

		[[nodiscard]] constexpr bool empty() const {
			return size() == 0;
		}

		// element access

		[[nodiscard]] constexpr reference front() const {
			if (std::is_constant_evaluated() && !(!empty())) {
				std::abort();
			}
			return *this->_ptr;
		}

		[[nodiscard]] constexpr reference back() const {
			if (std::is_constant_evaluated() && !(!empty())) {
				std::abort();
			}
			return *(this->_ptr + (size() - 1));
		}

		[[nodiscard]] constexpr reference operator[](size_type __idx) const {
			if (std::is_constant_evaluated() && !(__idx < size())) {
				std::abort();
			}
			return *(this->_ptr + __idx);
		}

		[[nodiscard]] constexpr pointer data() const {
			return this->_ptr;
		}

		[[nodiscard]] constexpr iterator begin() const {
			return iterator(this->_ptr);
		}

		[[nodiscard]] constexpr iterator end() const {
			return iterator(this->_ptr + this->size());
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() const {
			return reverse_iterator(this->end());
		}

		[[nodiscard]] constexpr reverse_iterator rend() const {
			return reverse_iterator(this->begin());
		}

		template<size_t Count>
		[[nodiscard]] constexpr span<element_type, Count> first() const {
			if constexpr (Extent == dynamic_extent) {
				if (std::is_constant_evaluated() && !(Count <= size())) {
					std::abort();
				}
			} else {
				static_assert(Count <= extent);
			}
			using result_type = span<element_type, Count>;
			return result_type { _SizedPtr { this->data() } };
		}

		[[nodiscard]] constexpr span<element_type, dynamic_extent> first(size_type count) const {
			if (std::is_constant_evaluated() && !(count <= size())) {
				std::abort();
			}
			return { this->data(), count };
		}

		template<size_t Count>
		[[nodiscard]] constexpr span<element_type, Count> last() const {
			if constexpr (Extent == dynamic_extent) {
				if (std::is_constant_evaluated() && !(Count <= size())) {
					std::abort();
				}
			} else {
				static_assert(Count <= extent);
			}
			using result_type = span<element_type, Count>;
			return result_type { _SizedPtr { this->data() + (this->size() - Count) } };
		}

		[[nodiscard]] constexpr span<element_type, dynamic_extent> last(size_type count) const {
			if (std::is_constant_evaluated() && !(count <= size())) {
				std::abort();
			}
			return { this->data() + (this->size() - count), count };
		}

		template<size_t Offset, size_t Count = dynamic_extent>
		[[nodiscard]] constexpr auto subspan() const -> span<element_type, subspan_extent<Offset, Count>()> {
			if constexpr (Extent == dynamic_extent) {
				if (std::is_constant_evaluated() && !(Offset <= size())) {
					std::abort();
				}
			} else {
				static_assert(Offset <= extent);
			}

			using result_type = span<element_type, subspan_extent<Offset, Count>()>;

			if constexpr (Count == dynamic_extent) {
				return result_type { this->data() + Offset, this->size() - Offset };
			} else {
				if constexpr (Extent == dynamic_extent) {
					if (std::is_constant_evaluated() && !(Count <= size())) {
						std::abort();
					}
					if (std::is_constant_evaluated() && !(Count <= (size() - Offset))) {
						std::abort();
					}
				} else {
					static_assert(Count <= extent);
					static_assert(Count <= (extent - Offset));
				}
				return result_type { _SizedPtr { this->data() + Offset } };
			}
		}

		[[nodiscard]] constexpr span<element_type, dynamic_extent> subspan( //
			size_type offset, size_type count = dynamic_extent
		) const {
			if (std::is_constant_evaluated() && !(offset <= size())) {
				std::abort();
			}
			if (count == dynamic_extent) {
				count = this->size() - offset;
			} else {
				if (std::is_constant_evaluated() && !(count <= size())) {
					std::abort();
				}
				if (std::is_constant_evaluated() && !(offset + count <= size())) {
					std::abort();
				}
			}
			return { this->data() + offset, count };
		}

		template<typename OtherElementType, std::size_t OtherExtent>
		constexpr explicit(extent != dynamic_extent && OtherExtent == dynamic_extent)
		operator std::span<OtherElementType, OtherExtent>() const {
			return { begin(), end() };
		}

	private:
		template<typename, std::size_t>
		friend struct span;

		struct _SizedPtr {
			T* const ptr;
		};

		OV_ALWAYS_INLINE constexpr explicit span(_SizedPtr __ptr)
		requires(extent != dynamic_extent)
			: _ptr(__ptr._ptr), _extent(_v<extent>) {}

		pointer _ptr;
		OV_NO_UNIQUE_ADDRESS extent_storage<extent> _extent;
	};

	template<typename T, size_t ArrayExtent>
	span(T (&)[ArrayExtent]) -> span<T, ArrayExtent>;

	template<typename T, size_t ArrayExtent>
	span(std::array<T, ArrayExtent>&) -> span<T, ArrayExtent>;

	template<typename T, size_t ArrayExtent>
	span(std::array<T, ArrayExtent> const&) -> span<const T, ArrayExtent>;

	template<std::contiguous_iterator It, typename End>
	span(It, End) -> span<std::remove_reference_t<std::iter_reference_t<It>>>;

	template<std::ranges::contiguous_range Range>
	span(Range&&) -> span<std::remove_reference_t<std::ranges::range_reference_t<Range&>>>;
}

namespace OpenVic {
	static constexpr std::size_t dynamic_extent = _detail::forwardable_span::dynamic_extent;

	template<class T, std::size_t Extent = dynamic_extent>
	using forwardable_span = _detail::forwardable_span::span<T, Extent>;
}

namespace std {
	namespace ranges {
		template<typename T, std::size_t Extent>
		inline constexpr bool enable_borrowed_range<OpenVic::forwardable_span<T, Extent>> = true;

		template<typename T, std::size_t Extent>
		inline constexpr bool enable_view<OpenVic::forwardable_span<T, Extent>> = true;
	}

	template<typename T, size_t Extent>
	[[nodiscard]] inline OpenVic::forwardable_span<
		const std::byte, Extent == dynamic_extent ? dynamic_extent : Extent * sizeof(T)>
	as_bytes(OpenVic::forwardable_span<T, Extent> sp) {
		auto data = reinterpret_cast<const std::byte*>(sp.data());
		auto size = sp.size_bytes();
		constexpr std::size_t extent = Extent == dynamic_extent ? dynamic_extent : Extent * sizeof(T);
		return OpenVic::forwardable_span<const std::byte, extent> { data, size };
	}

	template<typename T, size_t Extent>
	requires(!std::is_const_v<T>)
	inline OpenVic::forwardable_span<std::byte, Extent == dynamic_extent ? dynamic_extent : Extent * sizeof(T)>
	as_writable_bytes [[nodiscard]] (OpenVic::forwardable_span<T, Extent> sp) {
		auto data = reinterpret_cast<std::byte*>(sp.data());
		auto size = sp.size_bytes();
		constexpr std::size_t extent = Extent == dynamic_extent ? dynamic_extent : Extent * sizeof(T);
		return OpenVic::forwardable_span<std::byte, extent> { data, size };
	}
}

#else
#include <span>

namespace OpenVic {
	template<class T, std::size_t Extent = std::dynamic_extent>
	using forwardable_span = std::span<T, Extent>;
}
#endif
