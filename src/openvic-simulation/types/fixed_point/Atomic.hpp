#pragma once

#include <atomic>
#include <utility>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace std {
	template<>
	struct atomic<OpenVic::fixed_point_t> : private atomic<decltype(std::declval<OpenVic::fixed_point_t>().get_raw_value())> {
		using value_type = OpenVic::fixed_point_t;
		using underlying_type = decltype(std::declval<value_type>().get_raw_value());
		using base_type = atomic<underlying_type>;

		atomic() noexcept = default;

		constexpr atomic(value_type fp) noexcept : base_type(fp.get_raw_value()) {}

		atomic& operator=(const atomic&) volatile = delete;
		atomic& operator=(const atomic&) = delete;

		operator value_type() const noexcept {
			return value_type::parse_raw(base_type::operator underlying_type());
		}
		operator value_type() const volatile noexcept {
			return value_type::parse_raw(base_type::operator underlying_type());
		}

		value_type operator=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator=(fp.get_raw_value()));
		}
		value_type operator=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator=(fp.get_raw_value()));
		}

		value_type operator++(int) noexcept {
			return value_type::parse_raw(base_type::fetch_add(value_type::_1().get_raw_value()));
		}
		value_type operator++(int) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_add(value_type::_1().get_raw_value()));
		}

		value_type operator--(int) noexcept {
			return value_type::parse_raw(base_type::fetch_sub(value_type::_1().get_raw_value()));
		}
		value_type operator--(int) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_sub(value_type::_1().get_raw_value()));
		}

		value_type operator++() noexcept {
			return value_type::parse_raw(base_type::operator+=(value_type::_1().get_raw_value()));
		}
		value_type operator++() volatile noexcept {
			return value_type::parse_raw(base_type::operator+=(value_type::_1().get_raw_value()));
		}

		value_type operator--() noexcept {
			return value_type::parse_raw(base_type::operator-=(value_type::_1().get_raw_value()));
		}
		value_type operator--() volatile noexcept {
			return value_type::parse_raw(base_type::operator-=(value_type::_1().get_raw_value()));
		}

		value_type operator+=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator+=(fp.get_raw_value()));
		}
		value_type operator+=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator+=(fp.get_raw_value()));
		}

		value_type operator-=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator-=(fp.get_raw_value()));
		}
		value_type operator-=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator-=(fp.get_raw_value()));
		}

		value_type operator&=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator&=(fp.get_raw_value()));
		}
		value_type operator&=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator&=(fp.get_raw_value()));
		}

		value_type operator|=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator|=(fp.get_raw_value()));
		}
		value_type operator|=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator|=(fp.get_raw_value()));
		}

		value_type operator^=(value_type fp) noexcept {
			return value_type::parse_raw(base_type::operator^=(fp.get_raw_value()));
		}
		value_type operator^=(value_type fp) volatile noexcept {
			return value_type::parse_raw(base_type::operator^=(fp.get_raw_value()));
		}

		OV_ALWAYS_INLINE value_type fetch_add(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::fetch_add(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type fetch_add(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_add(fp.get_raw_value(), m));
		}

		OV_ALWAYS_INLINE value_type fetch_sub(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::fetch_sub(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type fetch_sub(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_sub(fp.get_raw_value(), m));
		}

		OV_ALWAYS_INLINE void store(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			base_type::store(fp.get_raw_value(), m);
		}
		OV_ALWAYS_INLINE void store(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			base_type::store(fp.get_raw_value(), m);
		}

		OV_ALWAYS_INLINE value_type load(memory_order m = memory_order_seq_cst) const noexcept {
			return value_type::parse_raw(base_type::load(m));
		}
		OV_ALWAYS_INLINE value_type load(memory_order m = memory_order_seq_cst) const volatile noexcept {
			return value_type::parse_raw(base_type::load(m));
		}

		OV_ALWAYS_INLINE value_type exchange(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::exchange(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type exchange(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::exchange(fp.get_raw_value(), m));
		}

		OV_ALWAYS_INLINE bool compare_exchange_weak( //
			value_type& fp1, value_type fp2, memory_order m1, memory_order m2
		) noexcept {
			return base_type::compare_exchange_weak(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m1, m2);
		}
		OV_ALWAYS_INLINE bool compare_exchange_weak( //
			value_type& fp1, value_type fp2, memory_order m1, memory_order m2
		) volatile noexcept {
			return base_type::compare_exchange_weak(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m1, m2);
		}

		OV_ALWAYS_INLINE bool compare_exchange_weak( //
			value_type& fp1, value_type fp2, memory_order m = memory_order_seq_cst
		) noexcept {
			return base_type::compare_exchange_weak(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m);
		}
		OV_ALWAYS_INLINE bool compare_exchange_weak( //
			value_type& fp1, value_type fp2, memory_order m = memory_order_seq_cst
		) volatile noexcept {
			return base_type::compare_exchange_weak(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m);
		}

		OV_ALWAYS_INLINE bool compare_exchange_strong( //
			value_type& fp1, value_type fp2, memory_order m1, memory_order m2
		) noexcept {
			return base_type::compare_exchange_strong(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m1, m2);
		}
		OV_ALWAYS_INLINE bool compare_exchange_strong( //
			value_type& fp1, value_type fp2, memory_order m1, memory_order m2
		) volatile noexcept {
			return base_type::compare_exchange_strong(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m1, m2);
		}

		OV_ALWAYS_INLINE bool
		compare_exchange_strong(value_type& fp1, value_type fp2, memory_order m = memory_order_seq_cst) noexcept {
			return base_type::compare_exchange_strong(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m);
		}
		OV_ALWAYS_INLINE bool compare_exchange_strong( //
			value_type& fp1, value_type fp2, memory_order m = memory_order_seq_cst
		) volatile noexcept {
			return base_type::compare_exchange_strong(*reinterpret_cast<underlying_type*>(&fp1), fp2.get_raw_value(), m);
		}

		OV_ALWAYS_INLINE value_type fetch_and(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::fetch_and(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type fetch_and(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_and(fp.get_raw_value(), m));
		}

		OV_ALWAYS_INLINE value_type fetch_or(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::fetch_or(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type fetch_or(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_or(fp.get_raw_value(), m));
		}

		OV_ALWAYS_INLINE value_type fetch_xor(value_type fp, memory_order m = memory_order_seq_cst) noexcept {
			return value_type::parse_raw(base_type::fetch_xor(fp.get_raw_value(), m));
		}
		OV_ALWAYS_INLINE value_type fetch_xor(value_type fp, memory_order m = memory_order_seq_cst) volatile noexcept {
			return value_type::parse_raw(base_type::fetch_xor(fp.get_raw_value(), m));
		}

#if __cpp_lib_atomic_wait
		OV_ALWAYS_INLINE void wait(value_type old, memory_order m = memory_order_seq_cst) const noexcept {
			base_type::wait(old.get_raw_value(), m);
		}
#endif
	};
}

namespace OpenVic {
	using atomic_fixed_point_t = std::atomic<fixed_point_t>;

	struct moveable_atomic_fixed_point_t {
	private:
		atomic_fixed_point_t value;

	public:
		OV_ALWAYS_INLINE moveable_atomic_fixed_point_t() = default;
		OV_ALWAYS_INLINE moveable_atomic_fixed_point_t(const atomic_fixed_point_t new_value) : value { new_value.load() } {}

		OV_ALWAYS_INLINE moveable_atomic_fixed_point_t(moveable_atomic_fixed_point_t&& other) {
			value.store(other.value);
		}

		OV_ALWAYS_INLINE fixed_point_t get_copy_of_value() const {
			return value;
		}

		OV_ALWAYS_INLINE fixed_point_t operator=(fixed_point_t const& rhs) noexcept {
			return value = rhs;
		}
		OV_ALWAYS_INLINE fixed_point_t operator=(fixed_point_t const& rhs) volatile noexcept {
			return value = rhs;
		}

		OV_ALWAYS_INLINE fixed_point_t operator++(int) noexcept {
			return value++;
		}
		OV_ALWAYS_INLINE fixed_point_t operator++(int) volatile noexcept {
			return value++;
		}

		OV_ALWAYS_INLINE fixed_point_t operator--(int) noexcept {
			return value--;
		}
		OV_ALWAYS_INLINE fixed_point_t operator--(int) volatile noexcept {
			return value--;
		}

		OV_ALWAYS_INLINE fixed_point_t operator++() noexcept {
			return ++value;
		}
		OV_ALWAYS_INLINE fixed_point_t operator++() volatile noexcept {
			return ++value;
		}

		OV_ALWAYS_INLINE fixed_point_t operator--() noexcept {
			return --value;
		}
		OV_ALWAYS_INLINE fixed_point_t operator--() volatile noexcept {
			return --value;
		}

		OV_ALWAYS_INLINE fixed_point_t operator+=(fixed_point_t const& rhs) noexcept {
			return value += rhs;
		}
		OV_ALWAYS_INLINE fixed_point_t operator+=(fixed_point_t const& rhs) volatile noexcept {
			return value += rhs;
		}

		OV_ALWAYS_INLINE fixed_point_t operator-=(fixed_point_t const& rhs) noexcept {
			return value -= rhs;
		}
		OV_ALWAYS_INLINE fixed_point_t operator-=(fixed_point_t const& rhs) volatile noexcept {
			return value -= rhs;
		}
	};
}
