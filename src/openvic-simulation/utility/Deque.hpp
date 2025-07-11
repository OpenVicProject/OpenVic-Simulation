// deque standard header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once
#if __has_include(<__msvc_string_view.hpp>)

#include <yvals_core.h>
#if _STL_COMPILER_PREPROCESSOR
#include <xmemory>

#if _HAS_CXX17
#include <xpolymorphic_allocator.h>
#endif // _HAS_CXX17

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

namespace OpenVic::utility::_detail::deque {
	using namespace ::std;

	template<class _Mydeque>
	class _Deque_unchecked_const_iterator {
	private:
		using _Size_type = typename _Mydeque::size_type;

		static constexpr int _Block_size = _Mydeque::_Block_size;

	public:
		using iterator_category = random_access_iterator_tag;

		using value_type = typename _Mydeque::value_type;
		using difference_type = typename _Mydeque::difference_type;
		using pointer = typename _Mydeque::const_pointer;
		using reference = const value_type&;

		_Deque_unchecked_const_iterator() noexcept : _Mycont(), _Myoff(0) {}

		_Deque_unchecked_const_iterator(_Size_type _Off, const _Container_base12* _Pdeque) noexcept
			: _Mycont(static_cast<const _Mydeque*>(_Pdeque)), _Myoff(_Off) {}

		_NODISCARD reference operator*() const noexcept {
			return _Mycont->_Subscript(_Myoff);
		}

		_NODISCARD pointer operator->() const noexcept {
			return pointer_traits<pointer>::pointer_to(**this);
		}

		_Deque_unchecked_const_iterator& operator++() noexcept {
			++_Myoff;
			return *this;
		}

		_Deque_unchecked_const_iterator operator++(int) noexcept {
			_Deque_unchecked_const_iterator _Tmp = *this;
			++_Myoff;
			return _Tmp;
		}

		_Deque_unchecked_const_iterator& operator--() noexcept {
			--_Myoff;
			return *this;
		}

		_Deque_unchecked_const_iterator operator--(int) noexcept {
			_Deque_unchecked_const_iterator _Tmp = *this;
			--_Myoff;
			return _Tmp;
		}

		_Deque_unchecked_const_iterator& operator+=(const difference_type _Off) noexcept {
			_Myoff = static_cast<_Size_type>(_Myoff + _Off);
			return *this;
		}

		_NODISCARD _Deque_unchecked_const_iterator operator+(const difference_type _Off) const noexcept {
			_Deque_unchecked_const_iterator _Tmp = *this;
			_Tmp += _Off;
			return _Tmp;
		}

		_NODISCARD friend _Deque_unchecked_const_iterator
		operator+(const difference_type _Off, _Deque_unchecked_const_iterator _Next) noexcept {
			_Next += _Off;
			return _Next;
		}

		_Deque_unchecked_const_iterator& operator-=(const difference_type _Off) noexcept {
			_Myoff = static_cast<_Size_type>(_Myoff - _Off);
			return *this;
		}

		_NODISCARD _Deque_unchecked_const_iterator operator-(const difference_type _Off) const noexcept {
			_Deque_unchecked_const_iterator _Tmp = *this;
			_Tmp -= _Off;
			return _Tmp;
		}

		_NODISCARD difference_type operator-(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return static_cast<difference_type>(_Myoff - _Right._Myoff);
		}

		_NODISCARD reference operator[](const difference_type _Off) const noexcept {
			return *(*this + _Off);
		}

		_NODISCARD bool operator==(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return _Myoff == _Right._Myoff;
		}

#if _HAS_CXX20
		_NODISCARD strong_ordering operator<=>(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return _Myoff <=> _Right._Myoff;
		}
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
		_NODISCARD bool operator!=(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return !(*this == _Right);
		}

		_NODISCARD bool operator<(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return _Myoff < _Right._Myoff;
		}

		_NODISCARD bool operator>(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return _Right < *this;
		}

		_NODISCARD bool operator<=(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return !(_Right < *this);
		}

		_NODISCARD bool operator>=(const _Deque_unchecked_const_iterator& _Right) const noexcept {
			return !(*this < _Right);
		}
#endif // ^^^ !_HAS_CXX20 ^^^

		const _Container_base12* _Getcont() const noexcept { // get container pointer
			return _Mycont;
		}

		const _Mydeque* _Mycont;
		_Size_type _Myoff; // offset of element in deque
	};

	template<class _Mydeque>
	class _Deque_unchecked_iterator : public _Deque_unchecked_const_iterator<_Mydeque> {
	private:
		using _Size_type = typename _Mydeque::size_type;
		using _Mybase = _Deque_unchecked_const_iterator<_Mydeque>;

	public:
		using iterator_category = random_access_iterator_tag;

		using value_type = typename _Mydeque::value_type;
		using difference_type = typename _Mydeque::difference_type;
		using pointer = typename _Mydeque::pointer;
		using reference = value_type&;

		using _Mybase::_Mybase;

		_NODISCARD reference operator*() const noexcept {
			return const_cast<reference>(_Mybase::operator*());
		}

		_NODISCARD pointer operator->() const noexcept {
			return pointer_traits<pointer>::pointer_to(**this);
		}

		_Deque_unchecked_iterator& operator++() noexcept {
			_Mybase::operator++();
			return *this;
		}

		_Deque_unchecked_iterator operator++(int) noexcept {
			_Deque_unchecked_iterator _Tmp = *this;
			_Mybase::operator++();
			return _Tmp;
		}

		_Deque_unchecked_iterator& operator--() noexcept {
			_Mybase::operator--();
			return *this;
		}

		_Deque_unchecked_iterator operator--(int) noexcept {
			_Deque_unchecked_iterator _Tmp = *this;
			_Mybase::operator--();
			return _Tmp;
		}

		_Deque_unchecked_iterator& operator+=(const difference_type _Off) noexcept {
			_Mybase::operator+=(_Off);
			return *this;
		}

		_NODISCARD _Deque_unchecked_iterator operator+(const difference_type _Off) const noexcept {
			_Deque_unchecked_iterator _Tmp = *this;
			_Tmp += _Off;
			return _Tmp;
		}

		_NODISCARD friend _Deque_unchecked_iterator
		operator+(const difference_type _Off, _Deque_unchecked_iterator _Next) noexcept {
			_Next += _Off;
			return _Next;
		}

		_Deque_unchecked_iterator& operator-=(const difference_type _Off) noexcept {
			_Mybase::operator-=(_Off);
			return *this;
		}

		_NODISCARD _Deque_unchecked_iterator operator-(const difference_type _Off) const noexcept {
			_Deque_unchecked_iterator _Tmp = *this;
			_Tmp -= _Off;
			return _Tmp;
		}

		_NODISCARD difference_type operator-(const _Mybase& _Right) const noexcept {
			return _Mybase::operator-(_Right);
		}

		_NODISCARD reference operator[](const difference_type _Off) const noexcept {
			return const_cast<reference>(_Mybase::operator[](_Off));
		}
	};

	template<class _Mydeque>
	class _Deque_const_iterator : public _Iterator_base12 {
	private:
		using _Size_type = typename _Mydeque::size_type;

		static constexpr int _Block_size = _Mydeque::_Block_size;

	public:
		using iterator_category = random_access_iterator_tag;

		using value_type = typename _Mydeque::value_type;
		using difference_type = typename _Mydeque::difference_type;
		using pointer = typename _Mydeque::const_pointer;
		using reference = const value_type&;

		using _Mydeque_t = _Mydeque; // helper for expression evaluator
		enum { _EEN_DS = _Block_size }; // helper for expression evaluator
		_Deque_const_iterator() noexcept : _Myoff(0) {
			_Setcont(nullptr);
		}

		_Deque_const_iterator(_Size_type _Off, const _Container_base12* _Pdeque) noexcept : _Myoff(_Off) {
			_Setcont(static_cast<const _Mydeque*>(_Pdeque));
		}

		_NODISCARD reference operator*() const noexcept {
			const auto _Mycont = static_cast<const _Mydeque*>(this->_Getcont());
#if _ITERATOR_DEBUG_LEVEL != 0
			_STL_VERIFY(_Mycont, "cannot dereference value-initialized deque iterator");
			_STL_VERIFY(
				_Mycont->_Myoff <= this->_Myoff && this->_Myoff < _Mycont->_Myoff + _Mycont->_Mysize,
				"cannot deference out of range deque iterator"
			);
#endif // _ITERATOR_DEBUG_LEVEL != 0

			return _Mycont->_Subscript(_Myoff);
		}

		_NODISCARD pointer operator->() const noexcept {
			return pointer_traits<pointer>::pointer_to(**this);
		}

		_Deque_const_iterator& operator++() noexcept {
#if _ITERATOR_DEBUG_LEVEL != 0
			const auto _Mycont = static_cast<const _Mydeque*>(this->_Getcont());
			_STL_VERIFY(_Mycont, "cannot increment value-initialized deque iterator");
			_STL_VERIFY(this->_Myoff < _Mycont->_Myoff + _Mycont->_Mysize, "cannot increment deque iterator past end");
#endif // _ITERATOR_DEBUG_LEVEL != 0

			++_Myoff;
			return *this;
		}

		_Deque_const_iterator operator++(int) noexcept {
			_Deque_const_iterator _Tmp = *this;
			++*this;
			return _Tmp;
		}

		_Deque_const_iterator& operator--() noexcept {
#if _ITERATOR_DEBUG_LEVEL != 0
			const auto _Mycont = static_cast<const _Mydeque*>(this->_Getcont());
			_STL_VERIFY(_Mycont, "cannot decrement value-initialized deque iterator");
			_STL_VERIFY(_Mycont->_Myoff < this->_Myoff, "cannot decrement deque iterator before begin");
#endif // _ITERATOR_DEBUG_LEVEL != 0

			--_Myoff;
			return *this;
		}

		_Deque_const_iterator operator--(int) noexcept {
			_Deque_const_iterator _Tmp = *this;
			--*this;
			return _Tmp;
		}

		_Deque_const_iterator& operator+=(const difference_type _Off) noexcept {
#if _ITERATOR_DEBUG_LEVEL != 0
			if (_Off != 0) {
				const auto _Mycont = static_cast<const _Mydeque*>(this->_Getcont());
				_STL_VERIFY(_Mycont, "cannot seek value-initialized deque iterator");
				_STL_VERIFY(
					_Mycont->_Myoff <= this->_Myoff + _Off && this->_Myoff + _Off <= _Mycont->_Myoff + _Mycont->_Mysize,
					"cannot seek deque iterator out of range"
				);
			}
#endif // _ITERATOR_DEBUG_LEVEL != 0

			_Myoff = static_cast<_Size_type>(_Myoff + _Off);
			return *this;
		}

		_NODISCARD _Deque_const_iterator operator+(const difference_type _Off) const noexcept {
			_Deque_const_iterator _Tmp = *this;
			_Tmp += _Off;
			return _Tmp;
		}

		_NODISCARD friend _Deque_const_iterator operator+(const difference_type _Off, _Deque_const_iterator _Next) noexcept {
			_Next += _Off;
			return _Next;
		}

		_Deque_const_iterator& operator-=(const difference_type _Off) noexcept {
			return *this += -_Off;
		}

		_NODISCARD _Deque_const_iterator operator-(const difference_type _Off) const noexcept {
			_Deque_const_iterator _Tmp = *this;
			_Tmp -= _Off;
			return _Tmp;
		}

		_NODISCARD difference_type operator-(const _Deque_const_iterator& _Right) const noexcept {
			_Compat(_Right);
			return static_cast<difference_type>(this->_Myoff - _Right._Myoff);
		}

		_NODISCARD reference operator[](const difference_type _Off) const noexcept {
			return *(*this + _Off);
		}

		_NODISCARD bool operator==(const _Deque_const_iterator& _Right) const noexcept {
			_Compat(_Right);
			return this->_Myoff == _Right._Myoff;
		}

#if _HAS_CXX20
		_NODISCARD strong_ordering operator<=>(const _Deque_const_iterator& _Right) const noexcept {
			_Compat(_Right);
			return this->_Myoff <=> _Right._Myoff;
		}
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
		_NODISCARD bool operator!=(const _Deque_const_iterator& _Right) const noexcept {
			return !(*this == _Right);
		}

		_NODISCARD bool operator<(const _Deque_const_iterator& _Right) const noexcept {
			_Compat(_Right);
			return this->_Myoff < _Right._Myoff;
		}

		_NODISCARD bool operator>(const _Deque_const_iterator& _Right) const noexcept {
			return _Right < *this;
		}

		_NODISCARD bool operator<=(const _Deque_const_iterator& _Right) const noexcept {
			return !(_Right < *this);
		}

		_NODISCARD bool operator>=(const _Deque_const_iterator& _Right) const noexcept {
			return !(*this < _Right);
		}
#endif // ^^^ !_HAS_CXX20 ^^^

		void _Compat(const _Deque_const_iterator& _Right) const noexcept { // test for compatible iterator pair
#if _ITERATOR_DEBUG_LEVEL == 0
			(void)_Right;
#else
			_STL_VERIFY(this->_Getcont() == _Right._Getcont(), "deque iterators incompatible");
#endif
		}

		void _Setcont(const _Mydeque* _Pdeque) noexcept { // set container pointer
			this->_Adopt(_Pdeque);
		}

		using _Prevent_inheriting_unwrap = _Deque_const_iterator;

		_NODISCARD _Deque_unchecked_const_iterator<_Mydeque> _Unwrapped() const noexcept {
			return { this->_Myoff, this->_Getcont() };
		}

		void _Verify_offset(const difference_type _Off) const noexcept {
#if _ITERATOR_DEBUG_LEVEL == 0
			(void)_Off;
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 0 / _ITERATOR_DEBUG_LEVEL != 0 vvv
			if (_Off != 0) {
				const auto _Mycont = static_cast<const _Mydeque*>(this->_Getcont());
				_STL_VERIFY(_Mycont, "cannot use value-initialized deque iterator");
				_STL_VERIFY(
					_Mycont->_Myoff <= this->_Myoff + _Off && this->_Myoff + _Off <= _Mycont->_Myoff + _Mycont->_Mysize,
					"cannot seek deque iterator out of range"
				);
			}
#endif // ^^^ _ITERATOR_DEBUG_LEVEL != 0 ^^^
		}

#if _ITERATOR_DEBUG_LEVEL != 0
		friend void _Verify_range(const _Deque_const_iterator& _First, const _Deque_const_iterator& _Last) noexcept {
			// note _Compat check inside operator<=
			_STL_VERIFY(_First <= _Last, "deque iterators transposed");
		}
#endif // _ITERATOR_DEBUG_LEVEL != 0

		void _Seek_to(const _Deque_unchecked_const_iterator<_Mydeque>& _UIt) noexcept {
			_Myoff = _UIt._Myoff;
		}

		_Size_type _Myoff; // offset of element in deque
	};

	template<class _Mydeque>
	class _Deque_iterator : public _Deque_const_iterator<_Mydeque> {
	private:
		using _Size_type = typename _Mydeque::size_type;
		using _Mybase = _Deque_const_iterator<_Mydeque>;

	public:
		using iterator_category = random_access_iterator_tag;

		using value_type = typename _Mydeque::value_type;
		using difference_type = typename _Mydeque::difference_type;
		using pointer = typename _Mydeque::pointer;
		using reference = value_type&;

		using _Mybase::_Mybase;

		_NODISCARD reference operator*() const noexcept {
			return const_cast<reference>(_Mybase::operator*());
		}

		_NODISCARD pointer operator->() const noexcept {
			return pointer_traits<pointer>::pointer_to(**this);
		}

		_Deque_iterator& operator++() noexcept {
			_Mybase::operator++();
			return *this;
		}

		_Deque_iterator operator++(int) noexcept {
			_Deque_iterator _Tmp = *this;
			_Mybase::operator++();
			return _Tmp;
		}

		_Deque_iterator& operator--() noexcept {
			_Mybase::operator--();
			return *this;
		}

		_Deque_iterator operator--(int) noexcept {
			_Deque_iterator _Tmp = *this;
			_Mybase::operator--();
			return _Tmp;
		}

		_Deque_iterator& operator+=(const difference_type _Off) noexcept {
			_Mybase::operator+=(_Off);
			return *this;
		}

		_NODISCARD _Deque_iterator operator+(const difference_type _Off) const noexcept {
			_Deque_iterator _Tmp = *this;
			_Tmp += _Off;
			return _Tmp;
		}

		_NODISCARD friend _Deque_iterator operator+(const difference_type _Off, _Deque_iterator _Next) noexcept {
			_Next += _Off;
			return _Next;
		}

		_Deque_iterator& operator-=(const difference_type _Off) noexcept {
			_Mybase::operator-=(_Off);
			return *this;
		}

		using _Mybase::operator-;

		_NODISCARD _Deque_iterator operator-(const difference_type _Off) const noexcept {
			_Deque_iterator _Tmp = *this;
			_Tmp -= _Off;
			return _Tmp;
		}

		_NODISCARD reference operator[](const difference_type _Off) const noexcept {
			return const_cast<reference>(_Mybase::operator[](_Off));
		}

		using _Prevent_inheriting_unwrap = _Deque_iterator;

		_NODISCARD _Deque_unchecked_iterator<_Mydeque> _Unwrapped() const noexcept {
			return { this->_Myoff, this->_Getcont() };
		}
	};

	template<
		class _Value_type, class _Size_type, class _Difference_type, class _Pointer, class _Const_pointer, class _Mapptr_type>
	struct _Deque_iter_types {
		using value_type = _Value_type;
		using size_type = _Size_type;
		using difference_type = _Difference_type;
		using pointer = _Pointer;
		using const_pointer = _Const_pointer;
		using _Mapptr = _Mapptr_type;
	};

	template<class _Ty>
	struct _Deque_simple_types : _Simple_types<_Ty> {
		using _Mapptr = _Ty**;
	};

	template<class _Val_types>
	class _Deque_val : public _Container_base12 {
	public:
		using value_type = typename _Val_types::value_type;
		using size_type = typename _Val_types::size_type;
		using difference_type = typename _Val_types::difference_type;
		using pointer = typename _Val_types::pointer;
		using const_pointer = typename _Val_types::const_pointer;
		using reference = value_type&;
		using const_reference = const value_type&;
		using _Mapptr = typename _Val_types::_Mapptr;

	private:
		using _Map_difference_type = typename iterator_traits<_Mapptr>::difference_type;

		static constexpr size_t _Bytes = sizeof(value_type);

	public:
		static constexpr int _Block_size = 512 /*_Bytes <= 1 ? 16
			: _Bytes <= 2							   ? 8
			: _Bytes <= 4							   ? 4
			: _Bytes <= 8							   ? 2
													   : 1*/
			; // elements per block (a power of 2)

		_Deque_val() noexcept : _Map(), _Mapsize(0), _Myoff(0), _Mysize(0) {}

		_Map_difference_type _Getblock(size_type _Off) const noexcept {
			// NB: _Mapsize and _Block_size are guaranteed to be powers of 2
			return static_cast<_Map_difference_type>((_Off / _Block_size) & (_Mapsize - 1));
		}

		reference _Subscript(size_type _Off) noexcept {
			const auto _Block = _Getblock(_Off);
			const auto _Block_off = static_cast<difference_type>(_Off % _Block_size);
			return _Map[_Block][_Block_off];
		}

		const_reference _Subscript(size_type _Off) const noexcept {
			const auto _Block = _Getblock(_Off);
			const auto _Block_off = static_cast<difference_type>(_Off % _Block_size);
			return _Map[_Block][_Block_off];
		}

		value_type* _Address_subscript(size_type _Off) noexcept {
			const auto _Block = _Getblock(_Off);
			const auto _Block_off = static_cast<difference_type>(_Off % _Block_size);
			return _STD _Unfancy(_Map[_Block] + _Block_off);
		}

		_Mapptr _Map; // pointer to array of pointers to blocks
		size_type _Mapsize; // size of map array, zero or 2^N
		size_type _Myoff; // offset of initial element
		size_type _Mysize; // current length of sequence
	};

	_EXPORT_STD template<class _Ty, class _Alloc = allocator<_Ty>>
	class deque {
	private:
		friend _Tidy_guard<deque>;
		static_assert(
			!_ENFORCE_MATCHING_ALLOCATORS || is_same_v<_Ty, typename _Alloc::value_type>,
			_MISMATCHED_ALLOCATOR_MESSAGE("deque<T, Allocator>", "T")
		);
		static_assert(
			is_object_v<_Ty>,
			"The C++ Standard forbids containers of non-object types "
			"because of [container.requirements]."
		);

		using _Alty = _Rebind_alloc_t<_Alloc, _Ty>;
		using _Alty_traits = allocator_traits<_Alty>;
		using _Alpty = _Rebind_alloc_t<_Alloc, typename _Alty_traits::pointer>;
		using _Alpty_traits = allocator_traits<_Alpty>;
		using _Mapptr = typename _Alpty_traits::pointer;
		using _Alproxy_ty = _Rebind_alloc_t<_Alty, _Container_proxy>;

		using _Map_difference_type = typename iterator_traits<_Mapptr>::difference_type;

		using _Scary_val = _Deque_val<conditional_t<
			_Is_simple_alloc_v<_Alty>, _Deque_simple_types<_Ty>,
			_Deque_iter_types<
				_Ty, typename _Alty_traits::size_type, typename _Alty_traits::difference_type, typename _Alty_traits::pointer,
				typename _Alty_traits::const_pointer, _Mapptr>>>;

		static constexpr int _Minimum_map_size = 8;
		static constexpr int _Block_size = _Scary_val::_Block_size;

	public:
		using allocator_type = _Alloc;
		using value_type = _Ty;
		using size_type = typename _Alty_traits::size_type;
		using difference_type = typename _Alty_traits::difference_type;
		using pointer = typename _Alty_traits::pointer;
		using const_pointer = typename _Alty_traits::const_pointer;
		using reference = _Ty&;
		using const_reference = const _Ty&;

		using iterator = _Deque_iterator<_Scary_val>;
		using const_iterator = _Deque_const_iterator<_Scary_val>;
		using _Unchecked_iterator = _Deque_unchecked_iterator<_Scary_val>;
		using _Unchecked_const_iterator = _Deque_unchecked_const_iterator<_Scary_val>;

		using reverse_iterator = _STD reverse_iterator<iterator>;
		using const_reverse_iterator = _STD reverse_iterator<const_iterator>;
		enum { _EEN_DS = _Block_size }; // helper for expression evaluator

		deque() : _Mypair(_Zero_then_variadic_args_t {}) {
			_Get_data()._Alloc_proxy(static_cast<_Alproxy_ty>(_Getal()));
		}

		explicit deque(const _Alloc& _Al) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Get_data()._Alloc_proxy(static_cast<_Alproxy_ty>(_Getal()));
		}

		explicit deque(_CRT_GUARDOVERFLOW size_type _Count, const _Alloc& _Al = _Alloc())
			: _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Alproxy_ty _Alproxy(_Getal());
			_Container_proxy_ptr12<_Alproxy_ty> _Proxy(_Alproxy, _Get_data());
			_Tidy_guard<deque> _Guard { this };
			resize(_Count);
			_Guard._Target = nullptr;
			_Proxy._Release();
		}

		deque(_CRT_GUARDOVERFLOW size_type _Count, const _Ty& _Val) : _Mypair(_Zero_then_variadic_args_t {}) {
			_Construct_n(_Count, _Val);
		}

#if _HAS_CXX17
		template<class _Alloc2 = _Alloc, enable_if_t<_Is_allocator<_Alloc2>::value, int> = 0>
#endif // _HAS_CXX17
		deque(_CRT_GUARDOVERFLOW size_type _Count, const _Ty& _Val, const _Alloc& _Al)
			: _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Construct_n(_Count, _Val);
		}

		template<class _Iter, enable_if_t<_Is_iterator_v<_Iter>, int> = 0>
		deque(_Iter _First, _Iter _Last) : _Mypair(_Zero_then_variadic_args_t {}) {
			_Construct(_First, _Last);
		}

		template<class _Iter, enable_if_t<_Is_iterator_v<_Iter>, int> = 0>
		deque(_Iter _First, _Iter _Last, const _Alloc& _Al) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Construct(_First, _Last);
		}

#if _HAS_CXX23
		template<_Container_compatible_range<_Ty> _Rng>
		deque(from_range_t, _Rng&& _Range) : _Mypair(_Zero_then_variadic_args_t {}) {
			_Construct(_RANGES _Ubegin(_Range), _RANGES _Uend(_Range));
		}

		template<_Container_compatible_range<_Ty> _Rng>
		deque(from_range_t, _Rng&& _Range, const _Alloc& _Al) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Construct(_RANGES _Ubegin(_Range), _RANGES _Uend(_Range));
		}
#endif // _HAS_CXX23

		deque(const deque& _Right)
			: _Mypair(_One_then_variadic_args_t {}, _Alty_traits::select_on_container_copy_construction(_Right._Getal())) {
			_Construct(_Right._Unchecked_begin(), _Right._Unchecked_end());
		}

		deque(const deque& _Right, const _Identity_t<_Alloc>& _Al) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Construct(_Right._Unchecked_begin(), _Right._Unchecked_end());
		}

		deque(deque&& _Right) : _Mypair(_One_then_variadic_args_t {}, _STD move(_Right._Getal())) {
			_Get_data()._Alloc_proxy(static_cast<_Alproxy_ty>(_Getal()));
			_Take_contents(_Right);
		}

		deque(deque&& _Right, const _Identity_t<_Alloc>& _Al) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			if constexpr (!_Alty_traits::is_always_equal::value) {
				if (_Getal() != _Right._Getal()) {
					_Construct(
						_STD make_move_iterator(_Right._Unchecked_begin()), _STD make_move_iterator(_Right._Unchecked_end())
					);
					return;
				}
			}

			_Get_data()._Alloc_proxy(static_cast<_Alproxy_ty>(_Getal()));
			_Take_contents(_Right);
		}

		deque(initializer_list<_Ty> _Ilist, const _Alloc& _Al = allocator_type()) : _Mypair(_One_then_variadic_args_t {}, _Al) {
			_Construct(_Ilist.begin(), _Ilist.end());
		}

	private:
		template<class _Iter, class _Sent>
		void _Construct(_Iter _First, const _Sent _Last) { // initialize from input range [_First, _Last)
			_Alproxy_ty _Alproxy(_Getal());
			_Container_proxy_ptr12<_Alproxy_ty> _Proxy(_Alproxy, _Get_data());

			_Tidy_guard<deque> _Guard { this };
			for (; _First != _Last; ++_First) {
				_Emplace_back_internal(*_First);
			}

			_Guard._Target = nullptr;
			_Proxy._Release();
		}

		void _Construct_n(size_type _Count, const _Ty& _Val) { // construct from _Count * _Val
			_Alproxy_ty _Alproxy(_Getal());
			_Container_proxy_ptr12<_Alproxy_ty> _Proxy(_Alproxy, _Get_data());

			_Tidy_guard<deque> _Guard { this };
			for (; _Count > 0; --_Count) {
				_Emplace_back_internal(_Val);
			}

			_Guard._Target = nullptr;
			_Proxy._Release();
		}

		void _Take_contents(deque& _Right) noexcept {
			// move from _Right, stealing its contents
			// pre: _Getal() == _Right._Getal()
			auto& _My_data = _Get_data();
			auto& _Right_data = _Right._Get_data();
			_My_data._Swap_proxy_and_iterators(_Right_data);
			_My_data._Map = _Right_data._Map;
			_My_data._Mapsize = _Right_data._Mapsize;
			_My_data._Myoff = _Right_data._Myoff;
			_My_data._Mysize = _Right_data._Mysize;

			_Right_data._Map = nullptr;
			_Right_data._Mapsize = 0;
			_Right_data._Myoff = 0;
			_Right_data._Mysize = 0;
		}

	public:
		~deque() noexcept {
			_Tidy();
			_Alproxy_ty _Proxy_allocator(_Getal());
			_Delete_plain_internal(_Proxy_allocator, _STD exchange(_Get_data()._Myproxy, nullptr));
		}

		deque& operator=(const deque& _Right) {
			if (this == _STD addressof(_Right)) {
				return *this;
			}

			auto& _Al = _Getal();
			auto& _Right_al = _Right._Getal();
			if constexpr (_Choose_pocca_v<_Alty>) {
				if (_Al != _Right_al) {
					_Tidy();
					_Get_data()._Reload_proxy(static_cast<_Alproxy_ty>(_Al), static_cast<_Alproxy_ty>(_Right_al));
				}
			}

			_Pocca(_Al, _Right_al);
			assign(_Right._Unchecked_begin(), _Right._Unchecked_end());

			return *this;
		}

		deque& operator=(deque&& _Right) noexcept(_Alty_traits::is_always_equal::value) {
			if (this == _STD addressof(_Right)) {
				return *this;
			}

			auto& _Al = _Getal();
			auto& _Right_al = _Right._Getal();
			constexpr auto _Pocma_val = _Choose_pocma_v<_Alty>;
			if constexpr (_Pocma_val == _Pocma_values::_Propagate_allocators) {
				if (_Al != _Right_al) {
					_Alproxy_ty _Alproxy(_Al);
					_Alproxy_ty _Right_alproxy(_Right_al);
					_Container_proxy_ptr12<_Alproxy_ty> _Proxy(_Right_alproxy, _Leave_proxy_unbound {});
					_Tidy();
					_Pocma(_Al, _Right_al);
					_Proxy._Bind(_Alproxy, _STD addressof(_Get_data()));
					_Take_contents(_Right);
					return *this;
				}
			} else if constexpr (_Pocma_val == _Pocma_values::_No_propagate_allocators) {
				if (_Al != _Right_al) {
					assign(
						_STD make_move_iterator(_Right._Unchecked_begin()), _STD make_move_iterator(_Right._Unchecked_end())
					);
					return *this;
				}
			}

			_Tidy();
			_Pocma(_Al, _Right_al);
			_Take_contents(_Right);

			return *this;
		}

		deque& operator=(initializer_list<_Ty> _Ilist) {
			assign(_Ilist.begin(), _Ilist.end());
			return *this;
		}

		template<class _Iter, enable_if_t<_Is_iterator_v<_Iter>, int> = 0>
		void assign(_Iter _First, _Iter _Last) {
			_STD _Adl_verify_range(_First, _Last);
			_Assign_range(_STD _Get_unwrapped(_First), _STD _Get_unwrapped(_Last));
		}

#if _HAS_CXX23
		template<_Container_compatible_range<_Ty> _Rng>
		void assign_range(_Rng&& _Range) {
			static_assert(
				assignable_from<_Ty&, _RANGES range_reference_t<_Rng>>,
				"Elements must be assignable from the range's reference type (N4993 [sequence.reqmts]/60)."
			);
			_Assign_range(_RANGES _Ubegin(_Range), _RANGES _Uend(_Range));
		}
#endif // _HAS_CXX23

		void assign(_CRT_GUARDOVERFLOW size_type _Count, const _Ty& _Val) { // assign _Count * _Val
			_Orphan_all();
			auto _Myfirst = _Unchecked_begin();
			const auto _Oldsize = _Mysize();
			auto _Assign_count = (_STD min)(_Count, _Oldsize);
			for (; _Assign_count > 0; --_Assign_count) {
				*_Myfirst = _Val;
				++_Myfirst;
			}

			const auto _Shrink_by = _Oldsize - _Assign_count;
			auto _Extend_by = _Count - _Assign_count;
			_Erase_last_n(_Shrink_by);
			for (; _Extend_by > 0; --_Extend_by) {
				_Emplace_back_internal(_Val);
			}
		}

		void assign(initializer_list<_Ty> _Ilist) {
			assign(_Ilist.begin(), _Ilist.end());
		}

		_NODISCARD allocator_type get_allocator() const noexcept {
			return static_cast<allocator_type>(_Getal());
		}

	private:
		template<class _Iter, class _Sent>
		void _Assign_range(_Iter _First, const _Sent _Last) {
			_Orphan_all();
			auto _Myfirst = _Unchecked_begin();
			const auto _Mylast = _Unchecked_end();
			// Reuse existing elements
			for (; _Myfirst != _Mylast; ++_Myfirst, (void)++_First) {
				if (_First == _Last) {
					_Erase_last_n(static_cast<size_type>(_Mylast - _Myfirst));
					return;
				}

				*_Myfirst = *_First;
			}

			// Allocate new elements for remaining tail of values
			for (; _First != _Last; ++_First) {
				_Emplace_back_internal(*_First);
			}
		}

	public:
		_NODISCARD iterator begin() noexcept {
			return iterator(_Myoff(), _STD addressof(_Get_data()));
		}

		_NODISCARD const_iterator begin() const noexcept {
			return const_iterator(_Myoff(), _STD addressof(_Get_data()));
		}

		_NODISCARD iterator end() noexcept {
			return iterator(_Myoff() + _Mysize(), _STD addressof(_Get_data()));
		}

		_NODISCARD const_iterator end() const noexcept {
			return const_iterator(_Myoff() + _Mysize(), _STD addressof(_Get_data()));
		}

		_Unchecked_iterator _Unchecked_begin() noexcept {
			return _Unchecked_iterator(_Myoff(), _STD addressof(_Get_data()));
		}

		_Unchecked_const_iterator _Unchecked_begin() const noexcept {
			return _Unchecked_const_iterator(_Myoff(), _STD addressof(_Get_data()));
		}

		_Unchecked_iterator _Unchecked_end() noexcept {
			return _Unchecked_iterator(_Myoff() + _Mysize(), _STD addressof(_Get_data()));
		}

		_Unchecked_const_iterator _Unchecked_end() const noexcept {
			return _Unchecked_const_iterator(_Myoff() + _Mysize(), _STD addressof(_Get_data()));
		}

		iterator _Make_iter(const_iterator _Where) const noexcept {
			return iterator(_Where._Myoff, _STD addressof(_Get_data()));
		}

		_NODISCARD reverse_iterator rbegin() noexcept {
			return reverse_iterator(end());
		}

		_NODISCARD const_reverse_iterator rbegin() const noexcept {
			return const_reverse_iterator(end());
		}

		_NODISCARD reverse_iterator rend() noexcept {
			return reverse_iterator(begin());
		}

		_NODISCARD const_reverse_iterator rend() const noexcept {
			return const_reverse_iterator(begin());
		}

		_NODISCARD const_iterator cbegin() const noexcept {
			return begin();
		}

		_NODISCARD const_iterator cend() const noexcept {
			return end();
		}

		_NODISCARD const_reverse_iterator crbegin() const noexcept {
			return rbegin();
		}

		_NODISCARD const_reverse_iterator crend() const noexcept {
			return rend();
		}

		_NODISCARD_EMPTY_MEMBER bool empty() const noexcept {
			return _Mysize() == 0;
		}

		_NODISCARD size_type size() const noexcept {
			return _Mysize();
		}

		_NODISCARD size_type max_size() const noexcept {
			return (_STD min)(static_cast<size_type>(_STD _Max_limit<difference_type>()), _Alty_traits::max_size(_Getal()));
		}

		void resize(_CRT_GUARDOVERFLOW size_type _Newsize) {
			_Orphan_all();
			while (_Mysize() < _Newsize) {
				_Emplace_back_internal();
			}

			while (_Mysize() > _Newsize) {
				pop_back();
			}
		}

		void resize(_CRT_GUARDOVERFLOW size_type _Newsize, const _Ty& _Val) {
			_Orphan_all();
			while (_Mysize() < _Newsize) {
				_Emplace_back_internal(_Val);
			}

			while (_Mysize() > _Newsize) {
				pop_back();
			}
		}

		void shrink_to_fit() {
			if (empty()) {
				if (_Map() != nullptr) {
					_Reset_map();
				}
				return;
			}

			const auto _Mask = static_cast<size_type>(_Mapsize() - 1);

			const auto _Unmasked_first_used_block_idx = static_cast<size_type>(_Myoff() / _Block_size);
			const auto _First_used_block_idx = static_cast<size_type>(_Unmasked_first_used_block_idx & _Mask);

			// (_Myoff() + _Mysize() - 1) is for the last element, i.e. the back() of the deque.
			// Divide by _Block_size to get the unmasked index of the last used block.
			// Add 1 to get the unmasked index of the first unused block.
			const auto _Unmasked_first_unused_block_idx =
				static_cast<size_type>(((_Myoff() + _Mysize() - 1) / _Block_size) + 1);

			const auto _First_unused_block_idx = static_cast<size_type>(_Unmasked_first_unused_block_idx & _Mask);

			// deallocate unused blocks, traversing over the circular buffer until the first used block index
			for (auto _Block_idx = _First_unused_block_idx; _Block_idx != _First_used_block_idx;
				 _Block_idx = static_cast<size_type>((_Block_idx + 1) & _Mask)) {
				auto& _Block_ptr = _Map()[static_cast<_Map_difference_type>(_Block_idx)];
				if (_Block_ptr != nullptr) {
					_Getal().deallocate(_Block_ptr, _Block_size);
					_Block_ptr = nullptr;
				}
			}

			const auto _Used_block_count =
				static_cast<size_type>(_Unmasked_first_unused_block_idx - _Unmasked_first_used_block_idx);

			size_type _New_block_count = _Minimum_map_size; // should be power of 2

			while (_New_block_count < _Used_block_count) {
				_New_block_count *= 2;
			}

			if (_New_block_count >= _Mapsize()) {
				return;
			}

			// worth shrinking the internal map, do it

			_Alpty _Almap(_Getal());
			const auto _New_map = _Almap.allocate(_New_block_count);

			_Orphan_all(); // the map can be shrunk, invalidate all iterators

			// transfer the ownership of blocks to pointers in the new map
			for (size_type _Block = 0; _Block != _Used_block_count; ++_Block) {
				const auto _New_block_idx = static_cast<_Map_difference_type>(_Block);
				const auto _Old_block_idx = static_cast<_Map_difference_type>((_First_used_block_idx + _Block) & _Mask);
				_STD _Construct_in_place(_New_map[_New_block_idx], _Map()[_Old_block_idx]);
			}

			// null out the rest of the new map
			_STD _Uninitialized_value_construct_n_unchecked1(
				_New_map + static_cast<_Map_difference_type>(_Used_block_count), _New_block_count - _Used_block_count
			);

			for (auto _Block = _Map_distance(); _Block > 0;) {
				--_Block;
				_STD _Destroy_in_place(_Map()[_Block]); // destroy pointer to block
			}
			_Almap.deallocate(_Map(), _Mapsize()); // free storage for map

			_Map() = _New_map;
			_Mapsize() = _New_block_count;
			_Myoff() %= _Block_size; // the first element is within block index 0 of the new map
		}

		_NODISCARD const_reference operator[](size_type _Pos) const noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(_Pos < _Mysize(), "deque subscript out of range");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(_Pos);
		}

		_NODISCARD reference operator[](size_type _Pos) noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(_Pos < _Mysize(), "deque subscript out of range");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(_Pos);
		}

		_NODISCARD const_reference at(size_type _Pos) const {
			if (_Mysize() <= _Pos) {
				_Xran();
			}

			return _Subscript(_Pos);
		}

		_NODISCARD reference at(size_type _Pos) {
			if (_Mysize() <= _Pos) {
				_Xran();
			}

			return _Subscript(_Pos);
		}

		_NODISCARD reference front() noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(!empty(), "front() called on empty deque");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(0);
		}

		_NODISCARD const_reference front() const noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(!empty(), "front() called on empty deque");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(0);
		}

		_NODISCARD reference back() noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(!empty(), "back() called on empty deque");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(_Mysize() - 1);
		}

		_NODISCARD const_reference back() const noexcept /* strengthened */ {
#if _CONTAINER_DEBUG_LEVEL > 0
			_STL_VERIFY(!empty(), "back() called on empty deque");
#endif // _CONTAINER_DEBUG_LEVEL > 0

			return _Subscript(_Mysize() - 1);
		}

	private:
		template<class... _Tys>
		void _Emplace_back_internal(_Tys&&... _Vals) {
			if ((_Myoff() + _Mysize()) % _Block_size == 0 && _Mapsize() <= (_Mysize() + _Block_size) / _Block_size) {
				_Growmap(1);
			}
			_Myoff() &= _Mapsize() * _Block_size - 1;
			const auto _Newoff = static_cast<size_type>(_Myoff() + _Mysize());
			const auto _Block = _Getblock(_Newoff);
			if (_Map()[_Block] == nullptr) {
				_Map()[_Block] = _Getal().allocate(_Block_size);
			}

			_Alty_traits::construct(_Getal(), _Get_data()._Address_subscript(_Newoff), _STD forward<_Tys>(_Vals)...);

			++_Mysize();
		}

		template<class... _Tys>
		void _Emplace_front_internal(_Tys&&... _Vals) {
			if (_Myoff() % _Block_size == 0 && _Mapsize() <= (_Mysize() + _Block_size) / _Block_size) {
				_Growmap(1);
			}
			_Myoff() &= _Mapsize() * _Block_size - 1;
			const auto _Newoff = static_cast<size_type>((_Myoff() != 0 ? _Myoff() : _Mapsize() * _Block_size) - 1);
			const auto _Block = _Getblock(_Newoff);
			if (_Map()[_Block] == nullptr) {
				_Map()[_Block] = _Getal().allocate(_Block_size);
			}

			_Alty_traits::construct(_Getal(), _Get_data()._Address_subscript(_Newoff), _STD forward<_Tys>(_Vals)...);

			_Myoff() = _Newoff;
			++_Mysize();
		}

	public:
		template<class... _Valty>
		reference emplace_front(_Valty&&... _Val) {
			_Orphan_all();
			_Emplace_front_internal(_STD forward<_Valty>(_Val)...);
			return front();
		}

		template<class... _Valty>
		reference emplace_back(_Valty&&... _Val) {
			_Orphan_all();
			_Emplace_back_internal(_STD forward<_Valty>(_Val)...);

			return back();
		}

		template<class... _Valty>
		iterator emplace(const_iterator _Where, _Valty&&... _Val) {
			const auto _Off = static_cast<size_type>(_Where - begin());

#if _ITERATOR_DEBUG_LEVEL == 2
			_STL_VERIFY(_Off <= _Mysize(), "deque emplace iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2

			_Orphan_all();
			if (_Off == 0) { // at the beginning
				_Emplace_front_internal(_STD forward<_Valty>(_Val)...);
			} else if (_Off == _Mysize()) { // at the end
				_Emplace_back_internal(_STD forward<_Valty>(_Val)...);
			} else {
				_Alloc_temporary2<_Alty> _Obj(_Getal(), _STD forward<_Valty>(_Val)...); // handle aliasing

				if (_Off <= _Mysize() / 2) { // closer to front
					_Emplace_front_internal(_STD move(*_Unchecked_begin()));

					auto _Moving_dst = _STD _Next_iter(_Unchecked_begin());
					auto _Moving_src_begin = _STD _Next_iter(_Moving_dst);
					auto _Moving_src_end = _Moving_dst + static_cast<difference_type>(_Off);

					auto _Moving_dst_end = _STD _Move_unchecked(_Moving_src_begin, _Moving_src_end, _Moving_dst);
					*_Moving_dst_end = _STD move(_Obj._Get_value());
				} else { // closer to back
					_Emplace_back_internal(_STD move(*_STD _Prev_iter(_Unchecked_end())));

					auto _Moving_dst_end = _STD _Prev_iter(_Unchecked_end());
					auto _Moving_src_end = _STD _Prev_iter(_Moving_dst_end);
					auto _Moving_src_begin = _Unchecked_begin() + static_cast<difference_type>(_Off);

					_STD _Move_backward_unchecked(_Moving_src_begin, _Moving_src_end, _Moving_dst_end);
					*_Moving_src_begin = _STD move(_Obj._Get_value());
				}
			}

			return begin() + static_cast<difference_type>(_Off);
		}

		void push_front(const _Ty& _Val) {
			_Orphan_all();
			_Emplace_front_internal(_Val);
		}

		void push_front(_Ty&& _Val) {
			_Orphan_all();
			_Emplace_front_internal(_STD move(_Val));
		}

		void push_back(const _Ty& _Val) {
			_Orphan_all();
			_Emplace_back_internal(_Val);
		}

		void push_back(_Ty&& _Val) {
			_Orphan_all();
			_Emplace_back_internal(_STD move(_Val));
		}

#if _HAS_CXX23
		template<_Container_compatible_range<_Ty> _Rng>
		void prepend_range(_Rng&& _Range) {
			_Orphan_all();

			const auto _Oldsize = _Mysize();
			_Restore_old_size_guard<_Pop_direction::_Front> _Guard { this, _Oldsize };
			if constexpr (_RANGES bidirectional_range<_Rng>) {
				const auto _UFirst = _RANGES _Ubegin(_Range);
				auto _ULast = _RANGES _Get_final_iterator_unwrapped(_Range);
				while (_UFirst != _ULast) {
					_Emplace_front_internal(*--_ULast); // prepend in order
				}
			} else {
				auto _UFirst = _RANGES _Ubegin(_Range);
				const auto _ULast = _RANGES _Uend(_Range);
				for (; _UFirst != _ULast; ++_UFirst) {
					_Emplace_front_internal(*_UFirst); // prepend flipped
				}

				const auto _Num = static_cast<difference_type>(_Mysize() - _Oldsize);
				_STD reverse(_Unchecked_begin(), _Unchecked_begin() + _Num);
			}
			_Guard._Container = nullptr;
		}

		template<_Container_compatible_range<_Ty> _Rng>
		void append_range(_Rng&& _Range) {
			_Orphan_all();

			const auto _Oldsize = _Mysize();
			auto _UFirst = _RANGES _Ubegin(_Range);
			const auto _ULast = _RANGES _Uend(_Range);

			_Restore_old_size_guard<_Pop_direction::_Back> _Guard { this, _Oldsize };
			for (; _UFirst != _ULast; ++_UFirst) {
				_Emplace_back_internal(*_UFirst);
			}
			_Guard._Container = nullptr;
		}
#endif // _HAS_CXX23

		iterator insert(const_iterator _Where, const _Ty& _Val) {
			return emplace(_Where, _Val);
		}

		iterator insert(const_iterator _Where, _Ty&& _Val) {
			return emplace(_Where, _STD move(_Val));
		}

		iterator insert(const_iterator _Where, _CRT_GUARDOVERFLOW size_type _Count, const _Ty& _Val) {
			// insert _Count * _Val at _Where
			size_type _Off = static_cast<size_type>(_Where - begin());
			_Insert_n(_Where, _Count, _Val);
			return begin() + static_cast<difference_type>(_Off);
		}

		template<class _Iter, enable_if_t<_Is_iterator_v<_Iter>, int> = 0>
		iterator insert(const_iterator _Where, _Iter _First, _Iter _Last) {
			// insert [_First, _Last) at _Where
			_STD _Adl_verify_range(_First, _Last);
			const size_type _Off = static_cast<size_type>(_Where - begin());
			return _Insert_range<static_cast<_Is_bidi>(_Is_cpp17_bidi_iter_v<_Iter>)>(
				_Off, _STD _Get_unwrapped(_First), _STD _Get_unwrapped(_Last)
			);
		}

#if _HAS_CXX23
		template<_Container_compatible_range<_Ty> _Rng>
		iterator insert_range(const_iterator _Where, _Rng&& _Range) {
			const size_type _Off = static_cast<size_type>(_Where - begin());

			if constexpr (_RANGES bidirectional_range<_Rng>) {
				return _Insert_range<_Is_bidi::_Yes>(
					_Off, _RANGES _Ubegin(_Range), _RANGES _Get_final_iterator_unwrapped(_Range)
				);
			} else {
				return _Insert_range<_Is_bidi::_Nope>(_Off, _RANGES _Ubegin(_Range), _RANGES _Uend(_Range));
			}
		}
#endif // _HAS_CXX23

		iterator insert(const_iterator _Where, initializer_list<_Ty> _Ilist) {
			return insert(_Where, _Ilist.begin(), _Ilist.end());
		}

	private:
		enum class _Pop_direction : bool {
			_Front,
			_Back,
		};

		template<_Pop_direction _Direction>
		struct _NODISCARD _Restore_old_size_guard {
			deque* _Container;
			const size_type _Oldsize;

			~_Restore_old_size_guard() { // restore old size, at least
				if (_Container) {
					while (_Oldsize < _Container->_Mysize()) {
						if constexpr (_Direction == _Pop_direction::_Front) {
							_Container->pop_front();
						} else {
							_Container->pop_back();
						}
					}
				}
			}
		};

		enum class _Is_bidi : bool { _Nope, _Yes };

		template<_Is_bidi _Bidi, class _Iter, class _Sent>
		iterator _Insert_range(const size_type _Off, _Iter _First, _Sent _Last) {
			// insert [_First, _Last) at begin() + _Off
			// Pre: _Bidi == _Is_bidi::_Yes implies that _Iter and _Sent are the same type, and that
			//      _Iter is either a bidirectional_iterator or a Cpp17BidirectionalIterator.

#if _ITERATOR_DEBUG_LEVEL == 2
			_STL_VERIFY(_Mysize() >= _Off, "deque insert iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2

			if (_First == _Last) {
				return begin() + static_cast<difference_type>(_Off);
			}

			const size_type _Oldsize = _Mysize();

			_Orphan_all();
			if (_Off <= _Oldsize / 2) { // closer to front, push to front then rotate
				_Restore_old_size_guard<_Pop_direction::_Front> _Guard { this, _Oldsize };
				if constexpr (_Bidi == _Is_bidi::_Yes) {
					while (_First != _Last) {
						_Emplace_front_internal(*--_Last); // prepend in order
					}
				} else {
					for (; _First != _Last; ++_First) {
						_Emplace_front_internal(*_First); // prepend flipped
					}
				}

				_Guard._Container = nullptr;

				const auto _Num = static_cast<difference_type>(_Mysize() - _Oldsize);
				const auto _Myfirst = _Unchecked_begin();
				const auto _Mymid = _Myfirst + _Num;
				if constexpr (_Bidi == _Is_bidi::_Nope) {
					_STD reverse(_Myfirst, _Mymid); // flip new stuff in place
				}
				_STD rotate(_Myfirst, _Mymid, _Mymid + static_cast<difference_type>(_Off));
				return begin() + static_cast<difference_type>(_Off);
			}

			_Restore_old_size_guard<_Pop_direction::_Back> _Guard { this, _Oldsize };
			for (; _First != _Last; ++_First) {
				_Emplace_back_internal(*_First);
			}

			_Guard._Container = nullptr;

			const auto _Myfirst = _Unchecked_begin();
			_STD rotate(
				_Myfirst + static_cast<difference_type>(_Off), _Myfirst + static_cast<difference_type>(_Oldsize),
				_Unchecked_end()
			);

			return begin() + static_cast<difference_type>(_Off);
		}

		void _Insert_n(const_iterator _Where, size_type _Count, const _Ty& _Val) { // insert _Count * _Val at _Where
			_Unchecked_iterator _Mid;
			size_type _Num;
			size_type _Off = static_cast<size_type>(_Where - begin());
			size_type _Oldsize = _Mysize();
			size_type _Rem = _Oldsize - _Off;

#if _ITERATOR_DEBUG_LEVEL == 2
			_STL_VERIFY(_Off <= _Oldsize, "deque insert iterator outside range");
#endif // _ITERATOR_DEBUG_LEVEL == 2

			if (_Off < _Rem) { // closer to front
				_Restore_old_size_guard<_Pop_direction::_Front> _Guard { this, _Oldsize };
				if (_Off < _Count) { // insert longer than prefix
					for (_Num = _Count - _Off; _Num > 0; --_Num) {
						push_front(_Val); // push excess values
					}
					for (_Num = _Off; _Num > 0; --_Num) {
						push_front(_Subscript(_Count - 1)); // push prefix
					}

					_Mid = _Unchecked_begin() + static_cast<difference_type>(_Count);
					_STD fill_n(_Mid, _Off, _Val); // fill in rest of values
				} else { // insert not longer than prefix
					for (_Num = _Count; _Num > 0; --_Num) {
						push_front(_Subscript(_Count - 1)); // push part of prefix
					}

					_Mid = _Unchecked_begin() + static_cast<difference_type>(_Count);
					_Alloc_temporary2<_Alty> _Tmp(_Getal(), _Val); // in case _Val is in sequence
					_STD move(
						_Mid + static_cast<difference_type>(_Count), _Mid + static_cast<difference_type>(_Off),
						_Mid
					); // copy rest of prefix
					_STD fill(
						_Unchecked_begin() + static_cast<difference_type>(_Off), _Mid + static_cast<difference_type>(_Off),
						_Tmp._Get_value()
					); // fill in values
				}
				_Guard._Container = nullptr;
			} else { // closer to back
				_Restore_old_size_guard<_Pop_direction::_Back> _Guard { this, _Oldsize };
				if (_Rem < _Count) { // insert longer than suffix
					_Orphan_all();
					for (_Num = _Count - _Rem; _Num > 0; --_Num) {
						_Emplace_back_internal(_Val); // push excess values
					}
					for (_Num = 0; _Num < _Rem; ++_Num) {
						_Emplace_back_internal(_Subscript(_Off + _Num)); // push suffix
					}

					_Mid = _Unchecked_begin() + static_cast<difference_type>(_Off);
					_STD fill_n(_Mid, _Rem, _Val); // fill in rest of values
				} else { // insert not longer than prefix
					for (_Num = 0; _Num < _Count; ++_Num) {
						_Emplace_back_internal(_Subscript(_Off + _Rem - _Count + _Num)); // push part of prefix
					}

					_Mid = _Unchecked_begin() + static_cast<difference_type>(_Off);
					_Alloc_temporary2<_Alty> _Tmp(_Getal(), _Val); // in case _Val is in sequence
					_STD move_backward(
						_Mid, _Mid + static_cast<difference_type>(_Rem - _Count),
						_Mid + static_cast<difference_type>(_Rem)
					); // copy rest of prefix
					_STD fill_n(_Mid, _Count, _Tmp._Get_value()); // fill in values
				}
				_Guard._Container = nullptr;
			}
		}

	public:
		void pop_front() noexcept /* strengthened */ {
#if _ITERATOR_DEBUG_LEVEL == 2
			if (empty()) {
				_STL_REPORT_ERROR("deque empty before pop");
			} else { // something to erase, do it
				_Orphan_off(_Myoff());
				_Alty_traits::destroy(_Getal(), _Get_data()._Address_subscript(_Myoff()));
				if (--_Mysize() == 0) {
					_Myoff() = 0;
				} else {
					++_Myoff();
				}
			}
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 2 / _ITERATOR_DEBUG_LEVEL < 2 vvv
			_Alty_traits::destroy(_Getal(), _Get_data()._Address_subscript(_Myoff()));
			if (--_Mysize() == 0) {
				_Myoff() = 0;
			} else {
				++_Myoff();
			}
#endif // ^^^ _ITERATOR_DEBUG_LEVEL < 2 ^^^
		}

		void pop_back() noexcept /* strengthened */ {
#if _ITERATOR_DEBUG_LEVEL == 2
			if (empty()) {
				_STL_REPORT_ERROR("deque empty before pop");
			} else { // something to erase, do it
				size_type _Newoff = _Myoff() + _Mysize() - 1;
				_Orphan_off(_Newoff);
				_Alty_traits::destroy(_Getal(), _Get_data()._Address_subscript(_Newoff));
				if (--_Mysize() == 0) {
					_Myoff() = 0;
				}
			}
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 2 / _ITERATOR_DEBUG_LEVEL < 2 vvv
			size_type _Newoff = _Myoff() + _Mysize() - 1;
			_Alty_traits::destroy(_Getal(), _Get_data()._Address_subscript(_Newoff));
			if (--_Mysize() == 0) {
				_Myoff() = 0;
			}
#endif // ^^^ _ITERATOR_DEBUG_LEVEL < 2 ^^^
		}

		iterator erase(const_iterator _Where) noexcept(is_nothrow_move_assignable_v<value_type>) /* strengthened */ {
			return erase(_Where, _Next_iter(_Where));
		}

		iterator erase(
			const_iterator _First_arg, const_iterator _Last_arg
		) noexcept(is_nothrow_move_assignable_v<value_type>) /* strengthened */ {
			iterator _First = _Make_iter(_First_arg);
			iterator _Last = _Make_iter(_Last_arg);

#if _ITERATOR_DEBUG_LEVEL == 2
			_STL_VERIFY(_First <= _Last && begin() <= _First && _Last <= end(), "deque erase iterator outside range");
			_STD _Adl_verify_range(_First, _Last);

			auto _Off = static_cast<size_type>(_First - begin());
			auto _Count = static_cast<size_type>(_Last - _First);
			bool _Moved = _Off > 0 && _Off + _Count < _Mysize();
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 2 / _ITERATOR_DEBUG_LEVEL < 2 vvv
			auto _Off = static_cast<size_type>(_First - begin());
			auto _Count = static_cast<size_type>(_Last - _First);
#endif // ^^^ _ITERATOR_DEBUG_LEVEL < 2 ^^^

			if (_Count == 0) {
				return _First;
			}

			auto _Unchecked_first = _First._Unwrapped();
			auto _Unchecked_last = _Last._Unwrapped();

			if (_Off < static_cast<size_type>(_Unchecked_end() - _Unchecked_last)) { // closer to front
				_STD move_backward(_Unchecked_begin(), _Unchecked_first, _Unchecked_last); // copy over hole
				for (; _Count > 0; --_Count) {
					pop_front(); // pop copied elements
				}
			} else { // closer to back
				_STD move(_Unchecked_last, _Unchecked_end(), _Unchecked_first); // copy over hole
				for (; _Count > 0; --_Count) {
					pop_back(); // pop copied elements
				}
			}

#if _ITERATOR_DEBUG_LEVEL == 2
			if (_Moved) {
				_Orphan_all();
			}
#endif // _ITERATOR_DEBUG_LEVEL == 2

			return begin() + static_cast<difference_type>(_Off);
		}

	private:
		void _Erase_last_n(size_type _Count) noexcept {
			for (; _Count > 0; --_Count) {
				pop_back();
			}
		}

	public:
		void clear() noexcept { // erase all
			_Tidy();
		}

		void swap(deque& _Right) noexcept /* strengthened */ {
			using _STD swap;
			if (this != _STD addressof(_Right)) {
				_Pocs(_Getal(), _Right._Getal());
				auto& _My_data = _Get_data();
				auto& _Right_data = _Right._Get_data();
				_My_data._Swap_proxy_and_iterators(_Right_data);
				swap(_My_data._Map, _Right_data._Map); // intentional ADL
				_STD swap(_My_data._Mapsize, _Right_data._Mapsize);
				_STD swap(_My_data._Myoff, _Right_data._Myoff);
				_STD swap(_My_data._Mysize, _Right_data._Mysize);
			}
		}

	private:
		[[noreturn]] static void _Xlen() {
			_Xlength_error("deque<T> too long");
		}

		[[noreturn]] static void _Xran() {
			_Xout_of_range("invalid deque<T> subscript");
		}

		void _Growmap(size_type _Count) { // grow map by at least _Count pointers, _Mapsize() a power of 2
			static_assert(_Minimum_map_size > 1, "The _Xlen() test should always be performed.");

			_Alpty _Almap(_Getal());
			size_type _Newsize = _Mapsize() > 0 ? _Mapsize() : 1;
			while (_Newsize - _Mapsize() < _Count || _Newsize < _Minimum_map_size) {
				// scale _Newsize to 2^N >= _Mapsize() + _Count
				if (max_size() / _Block_size - _Newsize < _Newsize) {
					_Xlen(); // result too long
				}

				_Newsize *= 2;
			}

			size_type _Allocsize = _Newsize;

			const auto _Myboff = static_cast<size_type>(_Myoff() / _Block_size);
			const auto _Map_off = static_cast<_Map_difference_type>(_Myboff);
			_Mapptr _Newmap = _Allocate_at_least_helper(_Almap, _Allocsize);
			_Mapptr _Myptr = _Newmap + _Map_off;
			_STL_ASSERT(_Allocsize >= _Newsize, "_Allocsize >= _Newsize");
			while (_Newsize <= _Allocsize / 2) {
				_Newsize *= 2;
			}

			_Count = _Newsize - _Mapsize();

			const auto _Map_count = static_cast<_Map_difference_type>(_Count);

			_Myptr = _STD uninitialized_copy(_Map() + _Map_off, _Map() + _Map_distance(), _Myptr); // copy initial to end
			if (_Myboff <= _Count) { // increment greater than offset of initial block
				_Myptr = _STD uninitialized_copy(_Map(), _Map() + _Map_off, _Myptr); // copy rest of old
				_Uninitialized_value_construct_n_unchecked1(_Myptr, _Count - _Myboff); // clear suffix of new
				_Uninitialized_value_construct_n_unchecked1(_Newmap, _Myboff); // clear prefix of new
			} else { // increment not greater than offset of initial block
				_STD uninitialized_copy(_Map(), _Map() + _Map_count, _Myptr); // copy more old
				_Myptr = _STD uninitialized_copy(_Map() + _Map_count, _Map() + _Map_off, _Newmap); // copy rest of old
				_Uninitialized_value_construct_n_unchecked1(_Myptr, _Count); // clear rest to initial block
			}

			if (_Map() != nullptr) {
				_Destroy_range(_Map(), _Map() + _Map_distance());
				_Almap.deallocate(_Map(), _Mapsize()); // free storage for old
			}

			_Map() = _Newmap; // point at new
			_Mapsize() += _Count;
		}

		void _Reset_map() noexcept {
			// pre: each block pointer is either null or pointing to a block without constructed elements

			for (auto _Block = _Map_distance(); _Block > 0;) { // free storage for a block and destroy pointer
				--_Block;
				auto& _Block_ptr = _Map()[_Block];
				if (_Block_ptr) { // free block
					_Getal().deallocate(_Block_ptr, _Block_size);
				}
				_STD _Destroy_in_place(_Block_ptr); // destroy pointer to block
			}

			_Alpty _Almap(_Getal());
			_Almap.deallocate(_Map(), _Mapsize()); // free storage for map

			_Map() = nullptr;
			_Mapsize() = 0;
		}

		void _Tidy() noexcept { // free all storage
			_Orphan_all();

			while (!empty()) {
				pop_back();
			}

			if (_Map() != nullptr) {
				_Reset_map();
			}

			_STL_INTERNAL_CHECK(_Mapsize() == 0); // null map should always be paired with zero mapsize
		}

#if _ITERATOR_DEBUG_LEVEL == 2
		void _Orphan_off(size_type _Offlo) const noexcept { // orphan iterators with specified offset(s)
			size_type _Offhigh = _Myoff() + _Mysize() <= _Offlo + 1 ? static_cast<size_type>(-1) : _Offlo;
			if (_Offlo == _Myoff()) {
				_Offlo = 0;
			}

			_Lockit _Lock(_LOCK_DEBUG);
			_Iterator_base12** _Pnext = &_Get_data()._Myproxy->_Myfirstiter;
			while (*_Pnext) {
				const auto _Pnextoff = static_cast<const_iterator&>(**_Pnext)._Myoff;
				if (_Pnextoff < _Offlo || _Offhigh < _Pnextoff) {
					_Pnext = &(*_Pnext)->_Mynextiter;
				} else { // orphan the iterator
					(*_Pnext)->_Myproxy = nullptr;
					*_Pnext = (*_Pnext)->_Mynextiter;
				}
			}
		}
#endif // _ITERATOR_DEBUG_LEVEL == 2

		_Map_difference_type _Getblock(size_type _Off) const noexcept {
			return _Get_data()._Getblock(_Off);
		}

		void _Orphan_all() noexcept {
			_Get_data()._Orphan_all();
		}

		_Alty& _Getal() noexcept {
			return _Mypair._Get_first();
		}

		const _Alty& _Getal() const noexcept {
			return _Mypair._Get_first();
		}

		_Scary_val& _Get_data() noexcept {
			return _Mypair._Myval2;
		}

		const _Scary_val& _Get_data() const noexcept {
			return _Mypair._Myval2;
		}

		_Mapptr& _Map() noexcept {
			return _Get_data()._Map;
		}

		const _Mapptr& _Map() const noexcept {
			return _Get_data()._Map;
		}

		size_type& _Mapsize() noexcept {
			return _Get_data()._Mapsize;
		}

		const size_type& _Mapsize() const noexcept {
			return _Get_data()._Mapsize;
		}

		size_type& _Myoff() noexcept {
			return _Get_data()._Myoff;
		}

		const size_type& _Myoff() const noexcept {
			return _Get_data()._Myoff;
		}

		size_type& _Mysize() noexcept {
			return _Get_data()._Mysize;
		}

		const size_type& _Mysize() const noexcept {
			return _Get_data()._Mysize;
		}

		_Map_difference_type _Map_distance() const noexcept {
			return static_cast<_Map_difference_type>(_Get_data()._Mapsize);
		}

		reference _Subscript(size_type _Pos) noexcept {
			return _Get_data()._Subscript(_Myoff() + _Pos);
		}

		const_reference _Subscript(size_type _Pos) const noexcept {
			return _Get_data()._Subscript(_Myoff() + _Pos);
		}

		_Compressed_pair<_Alty, _Scary_val> _Mypair;
	};

#if _HAS_CXX17
	template<
		class _Iter, class _Alloc = allocator<_Iter_value_t<_Iter>>,
		enable_if_t<conjunction_v<_Is_iterator<_Iter>, _Is_allocator<_Alloc>>, int> = 0>
	deque(_Iter, _Iter, _Alloc = _Alloc()) -> deque<_Iter_value_t<_Iter>, _Alloc>;
#endif // _HAS_CXX17

#if _HAS_CXX23
	template<_RANGES input_range _Rng, _Allocator_for_container _Alloc = allocator<_RANGES range_value_t<_Rng>>>
	deque(from_range_t, _Rng&&, _Alloc = _Alloc()) -> deque<_RANGES range_value_t<_Rng>, _Alloc>;
#endif // _HAS_CXX23

	_EXPORT_STD template<class _Ty, class _Alloc>
	void swap(deque<_Ty, _Alloc>& _Left, deque<_Ty, _Alloc>& _Right) noexcept /* strengthened */ {
		_Left.swap(_Right);
	}

	_EXPORT_STD template<class _Ty, class _Alloc>
	_NODISCARD bool operator==(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return _Left.size() == _Right.size() &&
			_STD equal(_Left._Unchecked_begin(), _Left._Unchecked_end(), _Right._Unchecked_begin());
	}

#if _HAS_CXX20
	_EXPORT_STD template<class _Ty, class _Alloc>
	_NODISCARD _Synth_three_way_result<_Ty> operator<=>(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return _STD lexicographical_compare_three_way(
			_Left._Unchecked_begin(), _Left._Unchecked_end(), _Right._Unchecked_begin(), _Right._Unchecked_end(),
			_Synth_three_way {}
		);
	}
#else // ^^^ _HAS_CXX20 / !_HAS_CXX20 vvv
	template<class _Ty, class _Alloc>
	_NODISCARD bool operator!=(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return !(_Left == _Right);
	}

	template<class _Ty, class _Alloc>
	_NODISCARD bool operator<(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return _STD lexicographical_compare(
			_Left._Unchecked_begin(), _Left._Unchecked_end(), _Right._Unchecked_begin(), _Right._Unchecked_end()
		);
	}

	template<class _Ty, class _Alloc>
	_NODISCARD bool operator<=(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return !(_Right < _Left);
	}

	template<class _Ty, class _Alloc>
	_NODISCARD bool operator>(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return _Right < _Left;
	}

	template<class _Ty, class _Alloc>
	_NODISCARD bool operator>=(const deque<_Ty, _Alloc>& _Left, const deque<_Ty, _Alloc>& _Right) {
		return !(_Left < _Right);
	}
#endif // ^^^ !_HAS_CXX20 ^^^

#if _HAS_CXX20
	_EXPORT_STD template<class _Ty, class _Alloc, class _Uty>
	deque<_Ty, _Alloc>::size_type erase(deque<_Ty, _Alloc>& _Cont, const _Uty& _Val) {
		return _STD _Erase_remove(_Cont, _Val);
	}

	_EXPORT_STD template<class _Ty, class _Alloc, class _Pr>
	deque<_Ty, _Alloc>::size_type erase_if(deque<_Ty, _Alloc>& _Cont, _Pr _Pred) {
		return _STD _Erase_remove_if(_Cont, _STD _Pass_fn(_Pred));
	}
#endif // _HAS_CXX20

#if _HAS_CXX17
	namespace pmr {
		_EXPORT_STD template<class _Ty>
		using deque = ::OpenVic::utility::_detail::deque::deque<_Ty, _STD pmr::polymorphic_allocator<_Ty>>;
	} // namespace pmr
#endif // _HAS_CXX17
}

#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _STL_COMPILER_PREPROCESSOR

namespace OpenVic::utility {
	template<class T, class Allocator = std::allocator<T>>
	using deque = ::OpenVic::utility::_detail::deque::deque<T, Allocator>;

	namespace pmr {
		template<class T>
		using deque = ::OpenVic::utility::_detail::deque::pmr::deque<T>;
	}
}

#else
#include <deque>

namespace OpenVic::utility {
	template<class T, class Allocator = std::allocator<T>>
	using deque = std::deque<T, Allocator>;

	namespace pmr {
		template<class T>
		using deque = deque<T, std::pmr::polymorphic_allocator<T>>;
	}
}
#endif

#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic::utility {
	template<typename T>
	struct is_trivially_relocatable<deque<T>> : std::true_type {};
}
