#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Deque.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic::utility {
	OV_ALWAYS_INLINE uint32_t halfbits_to_floatbits(uint16_t p_half) {
		uint16_t h_exp, h_sig;
		uint32_t f_sgn, f_exp, f_sig;

		h_exp = (p_half & 0x7c00u);
		f_sgn = ((uint32_t)p_half & 0x8000u) << 16;
		switch (h_exp) {
		case 0x0000u: /* 0 or subnormal */
			h_sig = (p_half & 0x03ffu);
			/* Signed zero */
			if (h_sig == 0) {
				return f_sgn;
			}
			/* Subnormal */
			h_sig <<= 1;
			while ((h_sig & 0x0400u) == 0) {
				h_sig <<= 1;
				h_exp++;
			}
			f_exp = ((uint32_t)(127 - 15 - h_exp)) << 23;
			f_sig = ((uint32_t)(h_sig & 0x03ffu)) << 13;
			return f_sgn + f_exp + f_sig;
		case 0x7c00u: /* inf or NaN */
			/* All-ones exponent and a copy of the significand */
			return f_sgn + 0x7f800000u + (((uint32_t)(p_half & 0x03ffu)) << 13);
		default: /* normalized */
			/* Just need to adjust the exponent and shift */
			return f_sgn + (((uint32_t)(p_half & 0x7fffu) + 0x1c000u) << 13);
		}
	}

	OV_ALWAYS_INLINE float halfptr_to_float(const uint16_t* p_half) {
		return std::bit_cast<float>(halfbits_to_floatbits(*p_half));
	}

	OV_ALWAYS_INLINE float half_to_float(const uint16_t p_half) {
		return halfptr_to_float(&p_half);
	}

	OV_ALWAYS_INLINE uint16_t make_half_float(float p_value) {
		uint32_t x = std::bit_cast<uint32_t>(p_value);
		uint32_t sign = (unsigned short)(x >> 31);
		uint32_t mantissa;
		uint32_t exponent;
		uint16_t hf;

		// get mantissa
		mantissa = x & ((1 << 23) - 1);
		// get exponent bits
		exponent = x & (0xFF << 23);
		if (exponent >= 0x47800000) {
			// check if the original single precision float number is a NaN
			if (mantissa && (exponent == (0xFF << 23))) {
				// we have a single precision NaN
				mantissa = (1 << 23) - 1;
			} else {
				// 16-bit half-float representation stores number as Inf
				mantissa = 0;
			}
			hf = (((uint16_t)sign) << 15) | (uint16_t)((0x1F << 10)) | (uint16_t)(mantissa >> 13);
		}
		// check if exponent is <= -15
		else if (exponent <= 0x38000000) {
			/*
			// store a denorm half-float value or zero
			exponent = (0x38000000 - exponent) >> 23;
			mantissa >>= (14 + exponent);

			hf = (((uint16_t)sign) << 15) | (uint16_t)(mantissa);
			*/
			hf = 0; // denormals do not work for 3D, convert to zero
		} else {
			hf = (((uint16_t)sign) << 15) | (uint16_t)((exponent - 0x38000000) >> 13) | (uint16_t)(mantissa >> 13);
		}

		return hf;
	}

	struct half_float {
		float value;
		constexpr half_float() = default;
		constexpr half_float(float value) : value(value) {}

		constexpr operator float&() {
			return value;
		}

		constexpr operator float const&() const {
			return value;
		}

		constexpr operator bool() const {
			return value;
		}

		constexpr half_float operator+() const {
			return +value;
		}

		constexpr half_float operator-() const {
			return -value;
		}

		constexpr half_float operator!() const {
			return !value;
		}

		constexpr half_float operator+(half_float const& rhs) const {
			return value + rhs.value;
		}

		constexpr half_float operator-(half_float const& rhs) const {
			return value - rhs.value;
		}

		constexpr half_float operator*(half_float const& rhs) const {
			return value * rhs.value;
		}

		constexpr half_float operator/(half_float const& rhs) const {
			return value / rhs.value;
		}

		constexpr half_float& operator+=(half_float const& rhs) {
			value += rhs.value;
			return *this;
		}

		constexpr half_float& operator-=(half_float const& rhs) {
			value -= rhs.value;
			return *this;
		}

		constexpr half_float& operator*=(half_float const& rhs) {
			value *= rhs.value;
			return *this;
		}

		constexpr half_float& operator/=(half_float const& rhs) {
			value /= rhs.value;
			return *this;
		}

		constexpr bool operator==(half_float const& rhs) const {
			return value == rhs.value;
		}

		constexpr auto operator<=>(half_float const& rhs) const {
			return value <=> rhs.value;
		}
	};

	static_assert(std::endian::native != std::endian::big || std::endian::native != std::endian::little, "HOW?!?!?!");

	template<typename T>
	concept HasEncode = requires(T const& value, std::span<uint8_t> span) {
		{ value.template encode<std::endian::native>(span) } -> std::same_as<size_t>;
	};

	template<typename T>
	concept Encodable =
		HasEncode<T> || utility::specialization_of<T, std::variant> || std::integral<T> || std::floating_point<T> ||
		std::is_enum_v<T> || std::convertible_to<T, std::string_view> || std::same_as<T, half_float> ||
		std::same_as<T, Timespan> || std::same_as<T, Date> || IsColour<T> || std::same_as<T, fixed_point_t> ||
		std::convertible_to<T, std::span<const uint8_t>> || utility::specialization_of<T, std::vector> ||
		utility::specialization_of<T, OpenVic::utility::deque> || utility::specialization_of_span<T> ||
		utility::specialization_of<T, std::pair> || utility::specialization_of<T, std::tuple> ||
		utility::specialization_of<T, vec2_t> || utility::specialization_of<T, vec3_t> ||
		utility::specialization_of<T, vec4_t> || (std::is_empty_v<T> && std::is_default_constructible_v<T>);

	template<std::endian Endian>
	static constexpr std::integral_constant<std::endian, Endian> endian_tag {};

	template<typename T, std::endian Endian = std::endian::native>
	requires Encodable<T> || Encodable<std::remove_cvref_t<T>>
	static inline size_t encode( //
		T const& value, std::span<uint8_t> span = {}, std::integral_constant<std::endian, Endian> endian = {}
	) {
		if constexpr (std::is_const_v<T> || std::is_volatile_v<T> || std::is_reference_v<T>) {
			return encode<std::remove_cvref_t<T>, Endian>(value, span);
		} else if constexpr (HasEncode<T>) {
			return value.template encode<Endian>(span);
		} else if constexpr (utility::specialization_of<T, std::variant>) {
			const size_t index = value.index();
			const size_t index_size = encode<uint32_t, Endian>(index);
			const size_t size = //
				std::visit(
					[](auto& arg) -> size_t {
						return encode<decltype(arg), Endian>(arg);
					},
					value
				) +
				index_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			size_t offset = encode<uint32_t, Endian>(index, span);
			std::visit(
				[&](auto& arg) {
					encode<decltype(arg), Endian>(arg, span.subspan(offset));
				},
				value
			);

			return size;
		} else if constexpr (std::same_as<T, bool>) {
			return encode<uint8_t, Endian>(static_cast<uint8_t>(value), span);
		} else if constexpr (std::integral<T>) {
			using unsigned_type = std::make_unsigned_t<T>;

			if (!span.empty()) {
				if constexpr (sizeof(T) == 1) {
					*span.data() = *std::bit_cast<unsigned_type*>(&value);
				} else {
					T copy;
					if constexpr (std::endian::native != Endian) {
						copy = std::bit_cast<T>(utility::byteswap(std::bit_cast<unsigned_type>(value)));
					} else {
						copy = value;
					}
					decltype(span)::pointer ptr = span.data();
					for (size_t i = 0; i < sizeof(T); i++) {
						*ptr = *std::bit_cast<unsigned_type*>(&copy) & 0xFF;
						ptr++;
						*std::bit_cast<unsigned_type*>(&copy) >>= 8;
					}
				}
				return span.size() >= sizeof(T) ? sizeof(T) : 0;
			}

			return sizeof(T);
		} else if constexpr (std::floating_point<T>) {
			using int_type = std::conditional_t<std::same_as<T, float>, uint32_t, uint64_t>;

			return encode<int_type, Endian>(std::bit_cast<int_type>(value), span);
		} else if constexpr (std::is_enum_v<T>) {
			using underlying_type = std::underlying_type_t<T>;

			return encode<underlying_type, Endian>(std::bit_cast<underlying_type>(value), span);
		} else if constexpr (std::convertible_to<T, std::string_view>) {
			const std::string_view str = static_cast<std::string_view>(value);
			const size_t str_size = str.size();
			const size_t length_size = encode<uint32_t, Endian>(str_size);
			const size_t size = str_size + length_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			uint8_t* ptr = span.data() + encode<uint32_t, Endian>(str_size, span);
			std::copy_n(reinterpret_cast<uint8_t const*>(str.data()), str_size, ptr);

			return size;
		} else if constexpr (std::same_as<T, half_float>) {
			return encode<uint16_t, Endian>(make_half_float(value.value), span);
		} else if constexpr (std::same_as<T, Timespan>) {
			return encode<int64_t, Endian>(value.to_int(), span);
		} else if constexpr (std::same_as<T, Date>) {
			return encode<Timespan, Endian>(value.get_timespan(), span);
		} else if constexpr (IsColour<T>) {
			return encode<typename T::integer_type, Endian>(static_cast<typename T::integer_type>(value), span);
		} else if constexpr (std::same_as<T, fixed_point_t>) {
			return encode<typename T::value_type, Endian>(value.get_raw_value(), span);
		} else if constexpr (std::convertible_to<T, std::span<const uint8_t>>) {
			const std::span<const uint8_t> inner_span = static_cast<std::span<const uint8_t>>(value);
			const size_t span_size = inner_span.size();
			const size_t length_size = encode<uint32_t, Endian>(span_size);
			const size_t size = span_size + length_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			uint8_t* ptr = span.data() + encode<uint32_t, Endian>(span_size, span);
			std::copy_n(inner_span.data(), span_size, ptr);

			return size;
		} else if constexpr (utility::specialization_of<T, std::vector> ||
							 utility::specialization_of<T, OpenVic::utility::deque> || utility::specialization_of_span<T>) {
			const size_t collection_size = value.size();
			const size_t length_size = encode<uint32_t, Endian>(collection_size);
			const size_t size =
				collection_size * encode<typename T::value_type, Endian>(value.empty() ? typename T::value_type() : value[0]) +
				length_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			size_t offset = encode<uint32_t, Endian>(collection_size, span);
			for (auto const& element : value) {
				offset += encode(element, span.subspan(offset), endian_tag<Endian>);
			}

			return size;
		} else if constexpr (utility::specialization_of<T, std::pair> || utility::specialization_of<T, std::tuple>) {
			static size_t size = std::apply(
				[](auto&&... args) {
					return (encode<decltype(args), Endian>(args) + ...);
				},
				value
			);

			if (span.empty()) {
				return size;
			}

			size_t offset = 0;
			std::apply(
				[&](auto&&... args) {
					(( //
						 offset += encode<decltype(args), Endian>(args, span.subspan(offset))
					 ),
					 ...);
				},
				value
			);

			return size;
		} else if constexpr (utility::specialization_of<T, vec2_t>) {
			if (span.empty()) {
				return encode<typename T::type, Endian>(value.x) + encode<typename T::type, Endian>(value.y);
			}

			size_t offset = encode<typename T::type, Endian>(value.x, span);
			return offset + encode<typename T::type, Endian>(value.y, span.subspan(offset));
		} else if constexpr (utility::specialization_of<T, vec3_t>) {
			if (span.empty()) {
				return encode<typename T::type, Endian>(value.x) + encode<typename T::type, Endian>(value.y) +
					encode<typename T::type, Endian>(value.z);
			}

			size_t offset = encode<typename T::type, Endian>(value.x, span);
			offset += encode<typename T::type, Endian>(value.y, span.subspan(offset));
			return offset + encode<typename T::type, Endian>(value.z, span.subspan(offset));
		} else if constexpr (utility::specialization_of<T, vec4_t>) {
			if (span.empty()) {
				return encode<typename T::type, Endian>(value.x) + encode<typename T::type, Endian>(value.y) +
					encode<typename T::type, Endian>(value.z) + encode<typename T::type, Endian>(value.w);
			}

			size_t offset = encode<typename T::type, Endian>(value.x, span);
			offset += encode<typename T::type, Endian>(value.y, span.subspan(offset));
			offset += encode<typename T::type, Endian>(value.z, span.subspan(offset));
			return offset + encode<typename T::type, Endian>(value.w, span.subspan(offset));
		} else if constexpr (std::is_empty_v<T> && std::is_default_constructible_v<T>) {
			return 0;
		}
	}

	template<std::size_t I>
	using index_t = std::integral_constant<std::size_t, I>;

	template<std::size_t I>
	constexpr index_t<I> index_v {};

	template<std::size_t... Is>
	using indexes_t = std::variant<index_t<Is>...>;

	template<std::size_t... Is>
	constexpr indexes_t<Is...> get_index(std::index_sequence<Is...>, std::size_t I) {
		constexpr indexes_t<Is...> retvals[] = { index_v<Is>... };
		return retvals[I];
	}

	template<std::size_t N>
	constexpr auto get_index(std::size_t I) {
		return get_index(std::make_index_sequence<N> {}, I);
	}

	template<typename T>
	concept HasDecode = requires(std::span<uint8_t> span, size_t& r_decode_count) {
		{ T::template decode<std::endian::native>(span, r_decode_count) } -> std::same_as<T>;
	};

	template<typename T>
	concept Decodable = HasDecode<T> || utility::specialization_of<T, std::variant> || std::integral<T> ||
		std::floating_point<T> || std::is_enum_v<T> || std::convertible_to<std::string_view, T> ||
		std::is_constructible_v<T, std::string_view> || std::same_as<T, half_float> || std::same_as<T, Timespan> ||
		std::same_as<T, Date> || IsColour<T> || std::same_as<T, fixed_point_t> ||
		std::is_constructible_v<T, uint8_t*, uint8_t*> || utility::specialization_of<T, std::vector> ||
		utility::specialization_of<T, OpenVic::utility::deque> || utility::specialization_of<T, std::pair> ||
		utility::specialization_of<T, std::tuple> || utility::specialization_of_std_array_of<T, uint8_t> ||
		utility::specialization_of<T, vec2_t> || utility::specialization_of<T, vec3_t> ||
		utility::specialization_of<T, vec4_t> || (std::is_empty_v<T> && std::is_default_constructible_v<T>);

	template<typename T, std::endian Endian = std::endian::native>
	requires Decodable<T> || Decodable<std::remove_cv_t<T>>
	static inline T decode( //
		std::span<uint8_t> span, size_t& r_decode_count, std::integral_constant<std::endian, Endian> endian = {}
	) {
		if constexpr (std::is_const_v<T> || std::is_volatile_v<T>) {
			return decode<std::remove_cv_t<T>, Endian>(span, r_decode_count);
		} else if constexpr (HasDecode<T>) {
			return T::template decode<Endian>(span, r_decode_count);
		} else if constexpr (utility::specialization_of<T, std::variant>) {
			auto index = get_index<std::variant_size_v<T>>(decode<uint32_t, Endian>(span, r_decode_count));
			size_t offset = r_decode_count;
			T value = std::visit(
				[&](auto INDEX) -> T {
					using assigned_type = std::variant_alternative_t<INDEX, T>;
					return decode<assigned_type, Endian>(span.subspan(offset), r_decode_count);
				},
				index
			);
			r_decode_count = offset + r_decode_count;
			return value;
		} else if constexpr (std::same_as<T, bool>) {
			return static_cast<T>(decode<uint8_t, Endian>(span, r_decode_count));
		} else if constexpr (std::integral<T>) {
			using unsigned_type = std::make_unsigned_t<T>;

			r_decode_count = sizeof(T);
			if (OV_unlikely(span.size() < r_decode_count)) {
				r_decode_count = 0;
				OV_ERR_FAIL_V({});
			}

			if constexpr (sizeof(T) == 1) {
				return std::bit_cast<T>(span[0]);
			} else {
				decltype(span)::pointer ptr = span.data();
				T u = 0;
				for (int i = 0; i < r_decode_count; i++) {
					T b = *ptr;
					b <<= (i * 8);
					u |= b;
					ptr++;
				}
				if constexpr (std::endian::native != Endian) {
					u = utility::byteswap(u);
				}
				return u;
			}
		} else if constexpr (std::floating_point<T>) {
			using int_type = std::conditional_t<std::same_as<T, float>, uint32_t, uint64_t>;

			return std::bit_cast<T>(decode<int_type, Endian>(span, r_decode_count));
		} else if constexpr (std::is_enum_v<T>) {
			using underlying_type = std::underlying_type_t<T>;

			return std::bit_cast<T>(decode<underlying_type, Endian>(span, r_decode_count));
		} else if constexpr (std::convertible_to<std::string_view, T>) {
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			OV_ERR_FAIL_INDEX_V(size, span.size(), {});
			std::string_view view { reinterpret_cast<std::string_view::value_type*>(span.data() + r_decode_count), size };
			r_decode_count += size;
			return static_cast<T>(view);
		} else if constexpr (std::is_constructible_v<T, std::string_view>) {
			return T { decode<std::string_view, Endian>(span, r_decode_count) };
		} else if constexpr (std::same_as<T, half_float>) {
			return { half_to_float(decode<uint16_t, Endian>(span, r_decode_count)) };
		} else if constexpr (std::same_as<T, Timespan>) {
			return T { decode<int64_t, Endian>(span, r_decode_count) };
		} else if constexpr (std::same_as<T, Date>) {
			return T { decode<Timespan, Endian>(span, r_decode_count) };
		} else if constexpr (IsColour<T>) {
			return T::from_integer(decode<typename T::integer_type, Endian>(span, r_decode_count));
		} else if constexpr (std::same_as<T, fixed_point_t>) {
			return T::parse_raw(decode<typename T::value_type, Endian>(span, r_decode_count));
		} else if constexpr (utility::specialization_of<T, std::vector> ||
							 utility::specialization_of<T, OpenVic::utility::deque>) {
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			OV_ERR_FAIL_INDEX_V(size, span.size(), T {});
			T value;
			if constexpr (requires { value.reserve(size); }) {
				value.reserve(size);
			}
			size_t offset = r_decode_count;
			for (size_t index = 0; index < size; index++) {
				value.emplace_back(decode<typename T::value_type, Endian>(span.subspan(offset), r_decode_count));
				offset += r_decode_count;
			}
			r_decode_count = offset;
			return value;
		} else if constexpr (std::is_constructible_v<T, uint8_t*, uint8_t*>) {
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			OV_ERR_FAIL_INDEX_V(size, span.size(), (T { (uint8_t*)nullptr, (uint8_t*)nullptr }));
			T value //
				{ //
				  reinterpret_cast<uint8_t*>(span.data() + r_decode_count), //
				  reinterpret_cast<uint8_t*>(span.data() + r_decode_count + size)
				};
			r_decode_count += size;
			return value;
		} else if constexpr (utility::specialization_of<T, std::pair> || utility::specialization_of<T, std::tuple>) {
			union {
				T tuple;
				bool dummy;
			} value = { .dummy = false };
			size_t offset = 0;
			std::apply(
				[&](auto&... args) {
					((args = decode<std::remove_reference_t<decltype(args)>, Endian>(span.subspan(offset), r_decode_count),
					  offset += r_decode_count),
					 ...);
				},
				value.tuple
			);
			r_decode_count = offset;

			return std::move(value.tuple);
		} else if constexpr (utility::specialization_of_std_array_of<T, uint8_t>) {
			T value;
			OV_ERR_FAIL_INDEX_V(value.size(), span.size(), {});
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			std::uninitialized_copy_n(span.subspan(r_decode_count).data(), size, value.data());
			r_decode_count += size;
			return value;
		} else if constexpr (utility::specialization_of<T, vec2_t>) {
			T value;
			value.x = decode<typename T::type, Endian>(span, r_decode_count);
			size_t offset = r_decode_count;
			value.y = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			r_decode_count += offset;
			return value;
		} else if constexpr (utility::specialization_of<T, vec3_t>) {
			T value;
			value.x = decode<typename T::type, Endian>(span, r_decode_count);
			size_t offset = r_decode_count;
			value.y = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			offset += r_decode_count;
			value.z = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			r_decode_count += offset;
			return value;
		} else if constexpr (utility::specialization_of<T, vec4_t>) {
			T value;
			value.x = decode<typename T::type, Endian>(span, r_decode_count);
			size_t offset = r_decode_count;
			value.y = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			offset += r_decode_count;
			value.z = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			offset += r_decode_count;
			value.w = decode<typename T::type, Endian>(span.subspan(offset), r_decode_count);
			r_decode_count += offset;
			return value;
		} else if constexpr (std::is_empty_v<T> && std::is_default_constructible_v<T>) {
			r_decode_count = 0;
			return T();
		}
	}
}
