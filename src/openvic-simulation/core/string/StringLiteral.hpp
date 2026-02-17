#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <type_traits>

#include <openvic-simulation/types/BasicIterator.hpp>

namespace OpenVic {
	template<std::size_t N, typename CharT, class Traits>
	struct string_literal;

	namespace detail {
		struct _string_literal {
		protected:
			[[nodiscard]] static constexpr auto _to_string(auto const& input) {
				return string_literal(input);
			}
			[[nodiscard]] static constexpr auto _concat(auto const&... input) {
				return string_literal(_to_string(input)...);
			}
		};
	}

	template<std::size_t N, typename CharT = char, class Traits = std::char_traits<CharT>>
	struct string_literal : detail::_string_literal {
		using value_type = CharT;
		using pointer = value_type*;
		using const_pointer = value_type const*;
		using reference = value_type&;
		using const_reference = value_type const&;
		using const_iterator = OpenVic::basic_iterator<const_pointer, string_literal>;
		using iterator = const_iterator;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using reverse_iterator = const_reverse_iterator;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using traits_type = Traits;

		static constexpr size_type npos = size_type(-1);

		static constexpr auto size = std::integral_constant<std::size_t, N - 1> {};
		static constexpr auto length = size;

		value_type _data[N];

		[[nodiscard]] constexpr string_literal() noexcept = default;

		[[nodiscard]] constexpr string_literal(const value_type (&literal)[N]) noexcept {
			for (auto i = 0u; i != N; i++) {
				_data[i] = literal[i];
			}
		}

		[[nodiscard]] constexpr string_literal(value_type c) noexcept : _data {} {
			_data[0] = c;
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept {
			return const_iterator(_data);
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept {
			return const_iterator(_data + size());
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
			return begin();
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept {
			return end();
		}
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
			return const_reverse_iterator(end());
		}
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
			return const_reverse_iterator(begin());
		}
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
			return rbegin();
		}
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
			return rend();
		}

		[[nodiscard]] constexpr const_reference front() const noexcept {
			return as_string_view().front();
		}
		[[nodiscard]] constexpr const_reference back() const noexcept {
			return as_string_view().back();
		}
		[[nodiscard]] constexpr const_reference at(size_type pos) const {
			return as_string_view().at(pos);
		}

		[[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept {
			return as_string_view()[pos];
		}

		[[nodiscard]] constexpr bool empty() const {
			return size() == 0;
		}

		template<size_type N2>
		[[nodiscard]] constexpr bool operator==(string_literal<N2, value_type, traits_type> const& other) const {
			return size() == other.size && compare(other) == 0;
		}

		[[nodiscard]] constexpr bool starts_with(std::basic_string_view<value_type, traits_type> sv) const noexcept {
			return as_string_view().starts_with(sv);
		}
		[[nodiscard]] constexpr bool starts_with(value_type ch) const noexcept {
			return as_string_view().starts_with(ch);
		}
		[[nodiscard]] constexpr bool starts_with(value_type const* s) const {
			return as_string_view().starts_with(s);
		}

		template<size_type N2>
		[[nodiscard]] constexpr bool starts_with(string_literal<N2, value_type, traits_type> const& other) const {
			return starts_with(other.as_string_view());
		}

		[[nodiscard]] constexpr bool ends_with(std::basic_string_view<value_type, traits_type> sv) const noexcept {
			return as_string_view().ends_with(sv);
		}
		[[nodiscard]] constexpr bool ends_with(value_type ch) const noexcept {
			return as_string_view().ends_with(ch);
		}
		[[nodiscard]] constexpr bool ends_with(value_type const* s) const {
			return as_string_view().ends_with(s);
		}

		template<size_type N2>
		[[nodiscard]] constexpr bool ends_with(string_literal<N2, value_type, traits_type> const& other) const {
			return ends_with(other.as_string_view());
		}

		[[nodiscard]] constexpr auto clear() const noexcept {
			return string_literal<0, value_type, traits_type> {};
		}

		[[nodiscard]] constexpr auto push_back(value_type c) const noexcept {
			return *this + string_literal { c };
		}

		[[nodiscard]] constexpr string_literal<size(), value_type, traits_type> pop_back() const noexcept {
			string_literal<size(), value_type, traits_type> result {};
			for (size_type i = 0u; i != size(); i++) {
				result._data[i] = _data[i];
			}
			return result;
		}

		template<size_type count>
		[[nodiscard]] constexpr string_literal<N + count, value_type, traits_type> append(value_type ch) const noexcept {
			string_literal<N + count, value_type, traits_type> result {};
			for (auto i = 0u; i != N; i++) {
				result._data[i] = _data[i];
			}

			for (auto i = N; i != N + count; i++) {
				result._data[i] = ch;
			}

			return result;
		}

		template<size_type N2>
		[[nodiscard]] constexpr auto append(string_literal<N2, value_type, traits_type> str) const noexcept {
			return *this + str;
		}

		template<size_type N2>
		[[nodiscard]] constexpr auto append(const value_type (&literal)[N2]) const noexcept {
			return *this + literal;
		}

		template<size_type pos = 0, size_type count = npos>
		[[nodiscard]] constexpr auto substr() const noexcept {
			static_assert(pos <= N, "pos must be less than or equal to N");
			constexpr size_type result_size = std::min(count - pos, N - pos);

			string_literal<result_size, value_type, traits_type> result {};
			for (size_type i = 0u, i2 = pos; i != result_size; i++, i2++) {
				result._data[i] = _data[i2];
			}
			return result;
		}

		[[nodiscard]] constexpr auto substr() const noexcept {
			return substr<>();
		}

		[[nodiscard]] constexpr std::basic_string_view<CharT, Traits> substr( //
			size_type pos, size_type count = npos
		) const noexcept {
			return as_string_view().substr(pos, count);
		}

		[[nodiscard]] constexpr size_type find(std::string_view str, size_type pos = 0) const noexcept {
			return as_string_view().find(str, pos);
		}

		template<size_type N2>
		[[nodiscard]] constexpr size_type find(const value_type (&literal)[N2], size_type pos = 0) const noexcept {
			return as_string_view().find(literal, pos, N2 - 1);
		}

		[[nodiscard]] constexpr size_type rfind(std::string_view str, size_type pos = 0) const noexcept {
			return as_string_view().rfind(str, pos);
		}

		template<size_type N2>
		[[nodiscard]] constexpr size_type rfind(const value_type (&literal)[N2], size_type pos = 0) const noexcept {
			return as_string_view().find(literal, pos, N2 - 1);
		}

		[[nodiscard]] constexpr int compare(std::string_view str) const noexcept {
			return as_string_view().compare(str);
		}

		template<size_type N2>
		[[nodiscard]] constexpr int compare(const value_type (&literal)[N2]) const noexcept {
			return as_string_view().compare(0, size(), literal, N2 - 1);
		}

		[[nodiscard]] constexpr operator std::basic_string_view<value_type, traits_type>() const {
			return as_string_view();
		}

		[[nodiscard]] constexpr std::basic_string_view<value_type, traits_type> as_string_view() const {
			return std::basic_string_view<value_type, traits_type>(_data, size());
		}

		[[nodiscard]] constexpr operator value_type const*() const {
			return c_str();
		}

		[[nodiscard]] constexpr value_type const* c_str() const {
			return _data;
		}

		[[nodiscard]] constexpr value_type const* data() const {
			return _data;
		}

		template<size_type N2>
		[[nodiscard]] constexpr auto operator+(string_literal<N2, value_type, traits_type> const& other) const noexcept {
			string_literal<size() + N2, value_type, traits_type> result {};
			for (size_type i = 0u; i != N; i++) {
				result._data[i] = _data[i];
			}

			for (size_type i = size(), i2 = 0; i2 != N2; i++, i2++) {
				result._data[i] = other._data[i2];
			}
			return result;
		}

		template<size_type N2>
		[[nodiscard]] constexpr auto operator+(const value_type (&rhs)[N2]) const noexcept {
			return *this + _to_string(rhs);
		}

		template<size_type N2>
		[[nodiscard]] friend constexpr auto operator+( //
			const value_type (&lhs)[N2], string_literal<N, value_type, traits_type> rhs
		) noexcept {
			return _to_string(lhs) + rhs;
		}
	};
	template<std::size_t N, typename CharT = char, class Traits = std::char_traits<CharT>>
	string_literal(const CharT (&)[N]) -> string_literal<N, CharT, Traits>;

	template<typename CharT = char, class Traits = std::char_traits<CharT>>
	string_literal(CharT) -> string_literal<1, CharT, Traits>;
}
