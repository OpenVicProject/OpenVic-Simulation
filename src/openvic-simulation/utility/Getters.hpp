#pragma once

#include <concepts>
#include <string>
#include <string_view>

#define REF_GETTERS(var) \
	constexpr decltype(var)& get_##var() { \
		return var; \
	} \
	constexpr decltype(var) const& get_##var() const { \
		return var; \
	}

namespace OpenVic {
	struct ReturnByValueProperty {};

	/*
	 * Template function used to choose the return type and provide the implementation
	 * for variable getters created using the PROPERTY macro.
	 */
	template<typename decl, typename T>
	inline constexpr decltype(auto) _get_property(const T& property) {
		if constexpr(std::is_reference_v<decl>) {
			/* Return const reference */
			return property;
		} else if constexpr (std::same_as<T, std::string>) {
			/* Return std::string_view looking at std::string */
			return std::string_view { property };
		} else if constexpr (
			std::integral<T> || std::floating_point<T> || std::is_enum<T>::value || std::derived_from<T, ReturnByValueProperty>
		) {
			/* Return value */
			return T { property };
		} else if constexpr(std::is_pointer<T>::value) {
			/* Return const pointer */
			return static_cast<std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>>(property);
		} else {
			/* Return const reference */
			return property;
		}
	}

/*
 * Use this on a variable delcaration to generate a getter function. It assumes the variable is private and so
 * sets the accessibility modifier state back to private after declaring the getter as public.
 * Examples:
 * 		int PROPERTY(x);					// int x;						int get_x() const;
 * 		const std::string PROPERTY(name);	// const std::string name;		std::string_view get_name() const;
 * 		std::vector<int> PROPERTY(sizes);	// std::vector<int> sizes;		std::vector<int> const& get_sizes() const;
 * 		uint8_t const* PROPERTY(data);		// uint8_t const* data;			uint8_t const* get_data() const;
 * 		colour_t* PROPERTY(pixels);			// colour_t* pixels;			colour_t const* get_pixels() const;
 * 		CultureGroup const& PROPERTY(group);// CultureGroup const& group;	CultureGroup const& get_group() const;
 * 		Province& PROPERTY(province);		// Province& province;			Province const& get_province() const;
 */
#define PROPERTY(NAME) \
	NAME; \
public: \
	auto get_##NAME() const -> decltype(OpenVic::_get_property<decltype(NAME)>(NAME)) { \
		return OpenVic::_get_property<decltype(NAME)>(NAME); \
	} \
private:

// TODO: Special logic to decide argument type and control assignment.
#define PROPERTY_RW(NAME) \
	PROPERTY(NAME) \
public: \
	void set_##NAME(decltype(NAME) new_##NAME) { \
		NAME = new_##NAME; \
	} \
private:

}
