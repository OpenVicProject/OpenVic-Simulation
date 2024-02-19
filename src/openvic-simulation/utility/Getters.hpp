#pragma once

#include <concepts>
#include <string>
#include <string_view>

#include <openvic-dataloader/detail/SelfType.hpp>
#include <openvic-dataloader/detail/TypeName.hpp>

#define OV_DETAIL_GET_TYPE_BASE_CLASS(CLASS) \
	static constexpr std::string_view get_type_static() { return ::ovdl::detail::type_name<CLASS>(); } \
	constexpr virtual std::string_view get_type() const = 0; \
	static constexpr std::string_view get_base_type_static() { return ::ovdl::detail::type_name<CLASS>(); } \
	constexpr virtual std::string_view get_base_type() const { return get_base_type_static(); } \
	template<typename T> constexpr bool is_type() const { \
		return get_type().compare(::ovdl::detail::type_name<T>()) == 0; } \
	template<typename T> constexpr bool is_derived_from() const { \
		return is_type<T>() || get_base_type().compare(::ovdl::detail::type_name<T>()) == 0; } \
	template<typename T> constexpr T* cast_to() { \
		if (is_derived_from<T>() || is_type<CLASS>()) return (static_cast<T*>(this)); \
		return nullptr; } \
	template<typename T> constexpr const T* const cast_to() const { \
		if (is_derived_from<T>() || is_type<CLASS>()) return (static_cast<const T*>(this)); \
		return nullptr; }

#define OV_DETAIL_GET_TYPE \
	struct _self_type_tag {}; \
	constexpr auto _self_type_helper()->decltype(::ovdl::detail::Writer<_self_type_tag, decltype(this)> {}); \
	using type = ::ovdl::detail::Read<_self_type_tag>; \
	static constexpr std::string_view get_type_static() { return ::ovdl::detail::type_name<type>(); } \
	constexpr std::string_view get_type() const override { \
		return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }

#define OV_DETAIL_GET_BASE_TYPE(CLASS) \
	static constexpr std::string_view get_base_type_static() { return ::ovdl::detail::type_name<CLASS>(); } \
	constexpr std::string_view get_base_type() const override { \
		return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }

/* Create const and non-const reference getters for a variable, applied to its name in its declaration, e
 * for example: GameManager PROPERTY_REF(game_manager); */
#define PROPERTY_REF(NAME) PROPERTY_REF_FULL(NAME, private)
#define PROPERTY_REF_FULL(NAME, ACCESS) \
	NAME; \
public: \
	constexpr decltype(NAME)& get_##NAME() { \
		return NAME; \
	} \
	constexpr decltype(NAME) const& get_##NAME() const { \
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
	inline constexpr decltype(auto) _get_property(const T& property) {
		if constexpr (std::is_reference_v<decl>) {
			/* Return const reference */
			return property;
		} else if constexpr (std::same_as<T, std::string>) {
			/* Return std::string_view looking at std::string */
			return std::string_view { property };
		} else if constexpr (
			std::integral<T> || std::floating_point<T> || std::is_enum_v<T> || ReturnByValue<T>
		) {
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
 *		int PROPERTY(x);					// int x;						int get_x() const;
 *		const std::string PROPERTY(name);	// const std::string name;		std::string_view get_name() const;
 *		std::vector<int> PROPERTY(sizes);	// std::vector<int> sizes;		std::vector<int> const& get_sizes() const;
 *		uint8_t const* PROPERTY(data);		// uint8_t const* data;			uint8_t const* get_data() const;
 *		colour_t* PROPERTY(pixels);			// colour_t* pixels;			colour_t const* get_pixels() const;
 *		CultureGroup const& PROPERTY(group);// CultureGroup const& group;	CultureGroup const& get_group() const;
 *		Province& PROPERTY(province);		// Province& province;			Province const& get_province() const;
 */
#define PROPERTY(NAME) PROPERTY_ACCESS(NAME, private)
#define PROPERTY_CUSTOM_PREFIX(NAME, PREFIX) PROPERTY_CUSTOM_NAME(NAME, PREFIX##_##NAME)
#define PROPERTY_CUSTOM_NAME(NAME, GETTER_NAME) PROPERTY_FULL(NAME, GETTER_NAME, private)
#define PROPERTY_ACCESS(NAME, ACCESS) PROPERTY_FULL(NAME, get_##NAME, ACCESS)
#define PROPERTY_FULL(NAME, GETTER_NAME, ACCESS) \
	NAME; \
public: \
	constexpr auto GETTER_NAME() const -> decltype(OpenVic::_get_property<decltype(NAME)>(NAME)) { \
		return OpenVic::_get_property<decltype(NAME)>(NAME); \
	} \
ACCESS:

// TODO: Special logic to decide argument type and control assignment.
#define PROPERTY_RW(NAME) PROPERTY_RW_ACCESS(NAME, private)
#define PROPERTY_RW_CUSTOM_NAME(NAME, GETTER_NAME, SETTER_NAME) PROPERTY_RW_FULL(NAME, GETTER_NAME, SETTER_NAME, private)
#define PROPERTY_RW_ACCESS(NAME, ACCESS) PROPERTY_RW_FULL(NAME, get_##NAME, set_##NAME, ACCESS)
#define PROPERTY_RW_FULL(NAME, GETTER_NAME, SETTER_NAME, ACCESS) \
	PROPERTY_FULL(NAME, GETTER_NAME, ACCESS) \
public: \
	constexpr void SETTER_NAME(decltype(NAME) new_##NAME) { \
		NAME = new_##NAME; \
	} \
ACCESS:
