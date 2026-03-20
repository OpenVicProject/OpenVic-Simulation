#pragma once

#include <functional>

#include <concepts/type_traits.hpp>

// bug fix for std::hash & std::equal_to to work with std::reference_wrapper
namespace std {
	template<typename T>
	struct hash<std::reference_wrapper<T>> {
		using BaseType = std::remove_cv_t<T>;

		[[nodiscard]] size_t operator()(T const& val) const noexcept {
			return std::hash<BaseType> {}(val);
		}
	};
}

// bug fix for concepts/type_traits.hpp to work with std::reference_wrapper
namespace concepts {
	namespace detail {
		template<typename T>
		constexpr bool _is_ref_wrapper = false;

		template<typename T>
		constexpr bool _is_ref_wrapper<std::reference_wrapper<T>> = true;

		template<typename R, typename T, typename RQual, typename TQual>
		concept _ref_wrap_common_reference_with = _is_ref_wrapper<R> && requires {
			typename std::common_reference_t<typename R::type&, TQual>;
		} && std::convertible_to<RQual, std::common_reference_t<typename R::type&, TQual>>;
	}

	template<typename R, typename T, template<typename> class RQual, template<typename> class TQual>
	requires detail::_ref_wrap_common_reference_with<R, T, RQual<R>, TQual<T>> &&
		(!detail::_ref_wrap_common_reference_with<T, R, TQual<T>, RQual<R>>)
	struct basic_common_reference<R, T, RQual, TQual> {
		using type = std::common_reference_t<typename R::type&, TQual<T>>;
	};

	template<typename T, typename R, template<typename> class TQual, template<typename> class RQual>
	requires detail::_ref_wrap_common_reference_with<R, T, RQual<R>, TQual<T>> &&
		(!detail::_ref_wrap_common_reference_with<T, R, TQual<T>, RQual<R>>)
	struct basic_common_reference<T, R, TQual, RQual> {
		using type = std::common_reference_t<typename R::type&, TQual<T>>;
	};
}
