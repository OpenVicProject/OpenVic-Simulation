#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic::utility {
	template<std::size_t... Idxs>
	constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>) {
		return std::array { str[Idxs]... };
	}

	template<typename T>
	constexpr auto type_name_array() {
#if defined(__clang__)
		constexpr auto prefix = std::string_view { "[T = " };
		constexpr auto suffix = std::string_view { "]" };
		constexpr auto function = std::string_view { __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
		constexpr auto prefix = std::string_view { "with T = " };
		constexpr auto suffix = std::string_view { "]" };
		constexpr auto function = std::string_view { __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
		constexpr auto prefix = std::string_view { "type_name_array<" };
		constexpr auto suffix = std::string_view { ">(void)" };
		constexpr auto function = std::string_view { __FUNCSIG__ };
#else
#error Unsupported compiler
#endif

		constexpr auto start = function.find(prefix) + prefix.size();
		constexpr auto end = function.rfind(suffix);

		static_assert(start < end);

		constexpr auto name = function.substr(start, (end - start));
		return substring_as_array(name, std::make_index_sequence<name.size()> {});
	}

	template<typename T>
	struct type_name_holder {
		static inline constexpr auto value = type_name_array<T>();
	};

	template<typename T>
	constexpr auto type_name() -> std::string_view {
		constexpr auto& value = type_name_holder<T>::value;
		return std::string_view { value.data(), value.size() };
	}

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
	template<typename T>
	struct Reader {
		friend auto adl_GetSelfType(Reader<T>);
	};

	template<typename T, typename U>
	struct Writer {
		friend auto adl_GetSelfType(Reader<T>) {
			return U {};
		}
	};
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif

	inline void adl_GetSelfType() {}

	template<typename T>
	using Read = std::remove_pointer_t<decltype(adl_GetSelfType(Reader<T> {}))>;
}

#define OV_DETAIL_GET_TYPE_BASE_CLASS(CLASS) \
	static constexpr std::string_view get_type_static() { \
		return ::OpenVic::utility::type_name<CLASS>(); \
	} \
	constexpr virtual std::string_view get_type() const = 0; \
	static constexpr std::string_view get_base_type_static() { \
		return ::OpenVic::utility::type_name<CLASS>(); \
	} \
	constexpr virtual std::string_view get_base_type() const { \
		return get_base_type_static(); \
	} \
	template<typename T> \
	constexpr bool is_type() const { \
		return get_type().compare(::OpenVic::utility::type_name<T>()) == 0; \
	} \
	template<typename T> \
	constexpr bool is_derived_from() const { \
		return is_type<T>() || get_base_type().compare(::OpenVic::utility::type_name<T>()) == 0; \
	} \
	template<typename T> \
	constexpr T* cast_to() { \
		if (is_derived_from<T>() || is_type<CLASS>()) { \
			return (static_cast<T*>(this)); \
		} \
		return nullptr; \
	} \
	template<typename T> \
	constexpr T const* const cast_to() const { \
		if (is_derived_from<T>() || is_type<CLASS>()) { \
			return (static_cast<T const*>(this)); \
		} \
		return nullptr; \
	}

#define OV_DETAIL_GET_TYPE \
	struct _self_type_tag {}; \
	constexpr auto _self_type_helper() -> decltype(::OpenVic::utility::Writer<_self_type_tag, decltype(this)> {}); \
	using type = ::OpenVic::utility::Read<_self_type_tag>; \
	static constexpr std::string_view get_type_static() { \
		return ::OpenVic::utility::type_name<type>(); \
	} \
	constexpr std::string_view get_type() const override { \
		return ::OpenVic::utility::type_name<std::decay_t<decltype(*this)>>(); \
	}

#define OV_DETAIL_GET_BASE_TYPE(CLASS) \
	static constexpr std::string_view get_base_type_static() { \
		return ::OpenVic::utility::type_name<CLASS>(); \
	} \
	constexpr std::string_view get_base_type() const override { \
		return ::OpenVic::utility::type_name<std::decay_t<decltype(*this)>>(); \
	}

/* Create const and non-const reference getters for a variable, applied to its name in its declaration, e
 * for example: GameManager PROPERTY_REF(game_manager); */
#define PROPERTY_REF(NAME) PROPERTY_REF_FULL(NAME, private)
#define PROPERTY_REF_FULL(NAME, ACCESS) \
	NAME; \
\
public: \
	[[nodiscard]] constexpr decltype(NAME)& get_##NAME() { \
		return NAME; \
	} \
	[[nodiscard]] constexpr decltype(NAME) const& get_##NAME() const { \
		return NAME; \
	} \
	ACCESS:

/* Create const and non-const getters for a variable, applied to its name in its declaration, e
 * for example: GameManager& PROPERTY_MOD(game_manager); */
#define PROPERTY_MOD(NAME) PROPERTY_MOD_FULL(NAME, private)
#define PROPERTY_MOD_FULL(NAME, ACCESS) \
	NAME; \
	static_assert(std::is_reference_v<decltype(NAME)> && !std::is_const_v<std::remove_reference_t<decltype(NAME)>>); \
\
public: \
	[[nodiscard]] constexpr decltype(NAME) get_##NAME() { \
		return NAME; \
	} \
	[[nodiscard]] constexpr std::remove_reference_t<decltype(NAME)> const& get_##NAME() const { \
		return NAME; \
	} \
	ACCESS:

namespace OpenVic {
	/* Any struct tagged with ov_return_by_value will be returned by value by PROPERTY-generated getter functions,
	 * instead of by const reference as structs are by default. Use this for small structs which don't contain any
	 * dynamically allocated memory, e.g. dates and colours. The tag must be public, as in the example below:
	 *
	 * public:
	 *     using ov_return_by_value = void;
	 */
	template<typename T>
	concept ReturnByValue = requires { typename T::ov_return_by_value; };

	/*
	 * Template function used to choose the return type and provide the implementation
	 * for variable getters created using the PROPERTY macro.
	 */
	template<typename decl, typename T>
	inline constexpr decltype(auto) _get_property(T const& property) {
		if constexpr (std::is_reference_v<decl>) {
			/* Return const reference */
			return property;
		} else if constexpr (std::same_as<T, std::string> || std::same_as<T, memory::string>) {
			/* Return std::string_view looking at std::string */
			return std::string_view { property };
		} else if constexpr (std::integral<T> || std::floating_point<T> || std::is_enum_v<T> || ReturnByValue<T>) {
			/* Return value */
			return T { property };
		} else if constexpr (std::is_pointer_v<T>) {
			/* Return const pointer */
			return static_cast<std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>>(property);
		} else {
			/* Return const reference */
			return property;
		}
	}
}

/*
 * Use this on a variable declaration to generate a getter function. PROPERTY assumes the variable is private and so
 * sets the accessibility modifier state back to private after declaring the getter as public; use PROPERTY_ACCESS to
 * manually specify the accessibility level, if your variable deviates from this norm; use PROPERTY_CUSTOM_NAME when
 * you wish to manually specify the getter name; use PROPERTY_FULL if you want to specify everything.
 * Examples:
 *      int PROPERTY(x);                     // int x;                     int get_x() const;
 *      const std::string PROPERTY(name);    // const std::string name;    std::string_view get_name() const;
 *      std::vector<int> PROPERTY(sizes);    // std::vector<int> sizes;    std::vector<int> const& get_sizes() const;
 *      uint8_t const* PROPERTY(data);       // uint8_t const* data;       uint8_t const* get_data() const;
 *      colour_t* PROPERTY(pixels);          // colour_t* pixels;          colour_t const* get_pixels() const;
 *      CultureGroup const& PROPERTY(group); // CultureGroup const& group; CultureGroup const& get_group() const;
 *      Province& PROPERTY(province);        // Province& province;        Province const& get_province() const;
 */
#define PROPERTY(NAME, ...) PROPERTY_ACCESS(NAME, private, __VA_ARGS__)
#define PROPERTY_CUSTOM_PREFIX(NAME, PREFIX, ...) PROPERTY_CUSTOM_NAME(NAME, PREFIX##_##NAME, __VA_ARGS__)
#define PROPERTY_CUSTOM_NAME(NAME, GETTER_NAME, ...) PROPERTY_FULL(NAME, GETTER_NAME, private, __VA_ARGS__)
#define PROPERTY_ACCESS(NAME, ACCESS, ...) PROPERTY_FULL(NAME, get_##NAME, ACCESS, __VA_ARGS__)
#define PROPERTY_FULL(NAME, GETTER_NAME, ACCESS, ...) \
	NAME __VA_OPT__(=) __VA_ARGS__; \
\
public: \
	[[nodiscard]] constexpr auto GETTER_NAME() const -> decltype(OpenVic::_get_property<decltype(NAME)>(NAME)) { \
		return OpenVic::_get_property<decltype(NAME)>(NAME); \
	} \
	ACCESS:

// TODO: Special logic to decide argument type and control assignment.
#define PROPERTY_RW(NAME, ...) PROPERTY_RW_ACCESS(NAME, private, __VA_ARGS__)
#define PROPERTY_RW_CUSTOM_NAME(NAME, GETTER_NAME, SETTER_NAME, ...) PROPERTY_RW_FULL(NAME, GETTER_NAME, SETTER_NAME, private, __VA_ARGS__)
#define PROPERTY_RW_ACCESS(NAME, ACCESS, ...) PROPERTY_RW_FULL(NAME, get_##NAME, set_##NAME, ACCESS, __VA_ARGS__)
#define PROPERTY_RW_FULL(NAME, GETTER_NAME, SETTER_NAME, ACCESS, ...) \
	PROPERTY_FULL(NAME, GETTER_NAME, ACCESS, __VA_ARGS__) \
public: \
	constexpr void SETTER_NAME(decltype(NAME) new_##NAME) { \
		NAME = new_##NAME; \
	} \
	ACCESS:

// Generates getters for a non-const pointer variable, one non-const returning a non-const pointer and one const returning
// a const pointer. Const pointer variables should use PROPERTY which generates only the const pointer getter. By default
// this will change accessibility back to private after declaring the getter as public; use PROPERTY_PTR_ACCESS to specify
// a different access modifier. You can optionally specify an initialisation value for the variable.
#define PROPERTY_PTR(NAME, ...) PROPERTY_PTR_ACCESS(NAME, private, __VA_ARGS__)
#define PROPERTY_PTR_ACCESS(NAME, ACCESS, ...) \
	NAME __VA_OPT__(=) __VA_ARGS__; \
	static_assert(std::is_pointer_v<decltype(NAME)> && !std::is_const_v<std::remove_pointer_t<decltype(NAME)>>); \
public: \
	[[nodiscard]] constexpr decltype(NAME) get_##NAME() { \
		return NAME; \
	} \
	[[nodiscard]] constexpr std::add_pointer_t<std::add_const_t<std::remove_pointer_t<decltype(NAME)>>> get_##NAME() const { \
		return NAME; \
	} \
ACCESS:
