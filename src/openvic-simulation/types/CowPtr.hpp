#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	/**
	 * A Copy-On-Write pointer
	 *
	 * Any unmodified copy refers to the same pointer in memory
	 * Any modified copy allocates a new memory pointer (given use_count() is not 1)
	 */
	template<typename T>
	class cow_ptr {
		struct payload;

	public:
		using element_type = T;
		using reference = element_type&;
		using const_reference = element_type const&;
		using pointer = element_type*;
		using const_pointer = element_type const*;

		OV_ALWAYS_INLINE cow_ptr() : data(new payload) {}

		template<typename U>
		requires(!std::same_as<std::decay_t<U>, cow_ptr>)
		OV_ALWAYS_INLINE explicit cow_ptr(U&& x) : data(new payload(std::forward<U>(x))) {}

		template<typename... Args>
		requires std::constructible_from<T, Args...>
		OV_ALWAYS_INLINE cow_ptr(std::in_place_t, Args&&... args) : data(new payload(std::forward<Args>(args)...)) {}

		template<typename U, typename... Args>
		requires std::constructible_from<T, std::initializer_list<U>&, Args...>
		OV_ALWAYS_INLINE cow_ptr(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
			: data(new payload(ilist, std::forward<Args>(args)...)) {}

		OV_ALWAYS_INLINE cow_ptr(cow_ptr const& x) : data(x.data) {
			++data->count;
		}

		OV_ALWAYS_INLINE cow_ptr(cow_ptr&& x) : data(x.data) {
			x.data = nullptr;
		}

		OV_ALWAYS_INLINE ~cow_ptr() {
			if (data && (--data->count == 0)) {
				delete data;
				data = nullptr;
			}
		}

		cow_ptr& operator=(cow_ptr const& x) {
			if (&x != this) {
				*this = cow_ptr(x);
			}
			return *this;
		}

		cow_ptr& operator=(cow_ptr&& x) {
			auto tmp = std::move(x);
			swap(*this, tmp);
			return *this;
		}

		OV_ALWAYS_INLINE reference write() {
			if (data && data->count != 1) {
				*this = cow_ptr(read());
			}
			return data->value;
		}

		OV_ALWAYS_INLINE const_reference read() const {
			return data->value;
		}

		OV_ALWAYS_INLINE const_pointer get() const {
			return &read();
		}

		OV_ALWAYS_INLINE const_reference operator*() const {
			return read();
		}

		OV_ALWAYS_INLINE const_pointer operator->() const {
			return get();
		}

		OV_ALWAYS_INLINE size_t use_count(std::memory_order memory_order = std::memory_order::relaxed) const {
			return data ? data->count.load(memory_order) : 0;
		}

		template<typename... Args>
		requires std::constructible_from<T, Args...>
		OV_ALWAYS_INLINE const_reference emplace(Args&&... args) {
			~cow_ptr();
			data = new payload(std::forward<Args>(args)...);
			return read();
		}

		template<typename U, typename... Args>
		requires std::constructible_from<T, std::initializer_list<U>&, Args&&...>
		OV_ALWAYS_INLINE const_reference emplace(std::initializer_list<U> ilist, Args&&... args) {
			~cow_ptr();
			data = new payload(ilist, std::forward<Args>(args)...);
			return read();
		}

		OV_ALWAYS_INLINE explicit operator bool() const {
			return data == nullptr;
		}

		friend inline void swap(cow_ptr& x, cow_ptr& y) {
			using std::swap;
			swap(x.data, y.data);
		}

	private:
		struct payload {
			payload() = default;

			template<typename... Args>
			explicit payload(Args&&... args) : value(std::forward<Args>(args)...) {}

			std::atomic_size_t count = 1;
			T value;
		};

		payload* data;
	};

	template<class T>
	cow_ptr<std::decay_t<T>> make_cow(T&& value) {
		return cow_ptr<std::decay_t<T>>(std::forward<T>(value));
	}

	template<class T, class... Args>
	requires std::constructible_from<T, Args...>
	cow_ptr<T> make_cow(Args&&... args) {
		return cow_ptr<T>(std::in_place, std::forward<Args>(args)...);
	}

	template<class T, class U, class... Args>
	requires std::constructible_from<T, std::initializer_list<U>&, Args...>
	cow_ptr<T> make_cow(std::initializer_list<U> ilist, Args&&... args) {
		return cow_ptr<T>(std::in_place, ilist, std::forward<Args>(args)...);
	}

	template<typename T, typename U>
	[[nodiscard]] inline bool operator==(cow_ptr<T> const& lhs, cow_ptr<U> const& rhs) {
		return lhs.get() == rhs.get();
	}

	template<typename T>
	[[nodiscard]] inline bool operator==(cow_ptr<T> const& lhs, std::nullptr_t) {
		return !lhs;
	}

	template<typename T, typename U>
	inline std::strong_ordering operator<=>(cow_ptr<T> const& lhs, cow_ptr<U> const& rhs) {
		return std::compare_three_way {}(lhs.get(), rhs.get());
	}

	template<typename T>
	inline std::strong_ordering operator<=>(cow_ptr<T> const& lhs, std::nullptr_t) {
		using pointer = typename cow_ptr<T>::element_type*;
		return std::compare_three_way {}(lhs.get(), static_cast<pointer>(nullptr));
	}

	namespace cow {
		template<utility::specialization_of<cow_ptr> T>
		typename T::element_type const& read(T& v) {
			return v.read();
		}

		template<utility::specialization_of<cow_ptr> T>
		typename T::element_type& write(T& v) {
			return v.write();
		}
	}
}
