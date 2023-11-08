#pragma once

#include <type_traits>

namespace OpenVic {
	template<typename E>
		struct is_scoped_enum final : std::bool_constant < requires {
		requires std::is_enum_v<E>;
		requires !std::is_convertible_v<E, std::underlying_type_t<E>>;
	} > {};

	template<class T>
	inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;

	template<typename T>
	concept IsScopedEnum = is_scoped_enum_v<T>;

	template<IsScopedEnum T>
	struct enable_bitfield final : std::false_type {
	};

	template<IsScopedEnum T>
	inline constexpr bool enable_bitfield_v = enable_bitfield<T>::value;

	template<typename T>
	concept EnumSupportBitfield = enable_bitfield_v<T>;
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline auto operator|(const T lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline auto operator&(const T lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline auto operator^(const T lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return static_cast<T>(static_cast<underlying_type>(lhs) ^ static_cast<underlying_type>(rhs));
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline auto operator~(const T lhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return static_cast<T>(~static_cast<underlying_type>(lhs));
}

template<OpenVic::EnumSupportBitfield T>
constexpr inline decltype(auto) operator|=(T& lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	lhs = static_cast<T>(static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
	return lhs;
}

template<OpenVic::EnumSupportBitfield T>
constexpr inline decltype(auto) operator&=(T& lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	lhs = static_cast<T>(static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
	return lhs;
}

template<OpenVic::EnumSupportBitfield T>
constexpr inline decltype(auto) operator^=(T& lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	lhs = static_cast<T>(static_cast<underlying_type>(lhs) ^ static_cast<underlying_type>(rhs));
	return lhs;
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline bool operator<<(const T lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return (lhs & rhs) == rhs;
}

template<OpenVic::EnumSupportBitfield T>
[[nodiscard]] constexpr inline bool operator>>(const T lhs, const T rhs) noexcept {
	using underlying_type = std::underlying_type_t<T>;
	return (lhs & rhs) == lhs;
}
