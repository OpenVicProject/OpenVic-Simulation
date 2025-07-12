#pragma once

#include <type_traits>

#include "openvic-simulation/types/HasIndex.hpp"

namespace OpenVic {
	/*
	Generated with help from Gemini.
	*/
	namespace _detail {
		// Helper struct to hold the results of our detection.
		// This struct will always have `value`, `type_tag`, and `index_t` members.
		template<bool IsHasIndex, typename DeducedTypeTag, typename DeducedIndexT>
		struct HasIndexParamsResult {
			static constexpr bool value = IsHasIndex;
			using type_tag = DeducedTypeTag;
			using index_t = DeducedIndexT;
		};

		// Helper struct to detect if T inherits from HasIndex and extract its template parameters.
		template <typename T>
		struct extract_has_index_params {
		private:
			// SFINAE-friendly detection:
			// If T is derived from OpenVic::HasIndex<U_TypeTag, U_IndexT>,
			// this overload is chosen and returns a specific result type.
			template <typename U_TypeTag, typename U_IndexT>
			static HasIndexParamsResult<true, U_TypeTag, U_IndexT> test(const OpenVic::HasIndex<U_TypeTag, U_IndexT>*);

			// Fallback for types not derived from HasIndex.
			// Returns a result type indicating no HasIndex base, with dummy types.
			static HasIndexParamsResult<false, void, void> test(...); // Using void as dummy types

		public:
			// The 'type' alias will be either HasIndexParamsResult<true, ...> or HasIndexParamsResult<false, ...>
			using type = decltype(test(std::declval<T*>()));

			// Expose the results directly from the 'type'
			static constexpr bool is_has_index_base = type::value;
			using type_tag = typename type::type_tag;
			using index_t = typename type::index_t;
		};


		// This is the main type trait we want.
		template <typename T, typename SpecificTypeTag>
		struct is_has_index_with_tag_final
			: std::conditional_t<
				extract_has_index_params<T>::is_has_index_base, // Check if T is derived from any HasIndex
				std::is_same<
					SpecificTypeTag,
					typename extract_has_index_params<T>::type_tag // Compare extracted TypeTag
				>,
				std::false_type // If not a HasIndex, it's false
			> {};
	}

	template <typename T, typename SpecificTypeTag>
	struct InheritsFromHasIndexWithTag : public _detail::is_has_index_with_tag_final<T, SpecificTypeTag> {};
	
	template <typename T>
	struct InheritsFromHasIndex : public _detail::is_has_index_with_tag_final<T, T> {};
}