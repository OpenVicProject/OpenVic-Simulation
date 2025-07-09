#pragma once

#include <string>
#include <string_view>

#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct ModifierEffect : HasIdentifier {
		friend struct ModifierManager;

		static constexpr size_t FORMAT_MULTIPLIER_BIT_COUNT = 2;
		static constexpr size_t FORMAT_DECIMAL_PLACES_BIT_COUNT = 2;
		static constexpr size_t FORMAT_SUFFIX_BIT_COUNT = 2;
		static constexpr size_t FORMAT_POS_NEG_BIT_COUNT = 1;

		static constexpr size_t FORMAT_MULTIPLIER_BIT_OFFSET = 0;
		static constexpr size_t FORMAT_DECIMAL_PLACES_BIT_OFFSET = FORMAT_MULTIPLIER_BIT_OFFSET + FORMAT_MULTIPLIER_BIT_COUNT;
		static constexpr size_t FORMAT_SUFFIX_BIT_OFFSET = FORMAT_DECIMAL_PLACES_BIT_OFFSET + FORMAT_DECIMAL_PLACES_BIT_COUNT;
		static constexpr size_t FORMAT_POS_NEG_BIT_OFFSET = FORMAT_SUFFIX_BIT_OFFSET + FORMAT_SUFFIX_BIT_COUNT;

		enum suffix_t : uint8_t {
			NO_SUFFIX = 0,
			PERCENT   = 1,
			DAYS      = 2,
			SPEED     = 3
		};

		enum class format_t : uint8_t {
			// 1 and 2 bits
			FORMAT_PART_x1   = 0 << FORMAT_MULTIPLIER_BIT_OFFSET,
			FORMAT_PART_x10  = 1 << FORMAT_MULTIPLIER_BIT_OFFSET,
			FORMAT_PART_x100 = 2 << FORMAT_MULTIPLIER_BIT_OFFSET,

			// 4 and 8 bits
			FORMAT_PART_0DP = 0 << FORMAT_DECIMAL_PLACES_BIT_OFFSET,
			FORMAT_PART_1DP = 1 << FORMAT_DECIMAL_PLACES_BIT_OFFSET,
			FORMAT_PART_2DP = 2 << FORMAT_DECIMAL_PLACES_BIT_OFFSET,
			FORMAT_PART_3DP = 3 << FORMAT_DECIMAL_PLACES_BIT_OFFSET,

			// 16 bit and 32 bit
			FORMAT_PART_NO_SUFFIX      = NO_SUFFIX << FORMAT_SUFFIX_BIT_OFFSET,
			FORMAT_PART_PERCENT_SUFFIX = PERCENT   << FORMAT_SUFFIX_BIT_OFFSET,
			FORMAT_PART_DAYS_SUFFIX    = DAYS      << FORMAT_SUFFIX_BIT_OFFSET,
			FORMAT_PART_SPEED_SUFFIX   = SPEED     << FORMAT_SUFFIX_BIT_OFFSET,

			// 64 bit
			FORMAT_PART_POS = 0 << FORMAT_POS_NEG_BIT_OFFSET,
			FORMAT_PART_NEG = 1 << FORMAT_POS_NEG_BIT_OFFSET,

			FORMAT_x1_0DP_POS = FORMAT_PART_x1 | FORMAT_PART_0DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_POS,
			FORMAT_x1_0DP_NEG = FORMAT_PART_x1 | FORMAT_PART_0DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_1DP_POS = FORMAT_PART_x1 | FORMAT_PART_1DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_POS,
			FORMAT_x1_1DP_NEG = FORMAT_PART_x1 | FORMAT_PART_1DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_1DP_PC_NEG = FORMAT_PART_x1 | FORMAT_PART_1DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_2DP_POS = FORMAT_PART_x1 | FORMAT_PART_2DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_POS,
			FORMAT_x1_2DP_NEG = FORMAT_PART_x1 | FORMAT_PART_2DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_2DP_PC_POS = FORMAT_PART_x1 | FORMAT_PART_2DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_POS,
			FORMAT_x1_2DP_PC_NEG = FORMAT_PART_x1 | FORMAT_PART_2DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_3DP_POS = FORMAT_PART_x1 | FORMAT_PART_3DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_POS,

			FORMAT_x10_2DP_PC_POS = FORMAT_PART_x10 | FORMAT_PART_2DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_POS,

			FORMAT_x100_0DP_POS = FORMAT_PART_x100 | FORMAT_PART_0DP | FORMAT_PART_NO_SUFFIX | FORMAT_PART_POS,

			FORMAT_x100_0DP_PC_POS = FORMAT_PART_x100 | FORMAT_PART_0DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_POS,
			FORMAT_x100_0DP_PC_NEG = FORMAT_PART_x100 | FORMAT_PART_0DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x100_1DP_PC_POS = FORMAT_PART_x100 | FORMAT_PART_1DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_POS,
			FORMAT_x100_1DP_PC_NEG = FORMAT_PART_x100 | FORMAT_PART_1DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x100_2DP_PC_POS = FORMAT_PART_x100 | FORMAT_PART_2DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_POS,
			FORMAT_x100_2DP_PC_NEG = FORMAT_PART_x100 | FORMAT_PART_2DP | FORMAT_PART_PERCENT_SUFFIX | FORMAT_PART_NEG,

			FORMAT_x1_0DP_DAYS_NEG = FORMAT_PART_x1 | FORMAT_PART_0DP | FORMAT_PART_DAYS_SUFFIX | FORMAT_PART_NEG,
			FORMAT_x1_2DP_SPEED_POS = FORMAT_PART_x1 | FORMAT_PART_2DP | FORMAT_PART_SPEED_SUFFIX | FORMAT_PART_POS,
		};

		enum class target_t : uint8_t {
			NO_TARGETS  = 0,
			COUNTRY     = 1 << 0,
			PROVINCE    = 1 << 1,
			UNIT        = 1 << 2,
			ALL_TARGETS = (1 << 3) - 1
		};

		static constexpr bool excludes_targets(target_t targets, target_t excluded_target);

		static memory::string target_to_string(target_t target);

		static memory::string make_default_modifier_effect_localisation_key(std::string_view identifier);

	private:
		const bool PROPERTY_CUSTOM_PREFIX(no_effect, has);
		const format_t PROPERTY(format);
		const target_t PROPERTY(targets);
		memory::string PROPERTY(localisation_key);

		// TODO - format/precision, e.g. 80% vs 0.8 vs 0.800, 2 vs 2.0 vs 200%

		ModifierEffect(
			std::string_view new_identifier, format_t new_format, target_t new_targets,
			std::string_view new_localisation_key, bool new_has_no_effect
		);

	public:
		ModifierEffect(ModifierEffect&&) = default;

		// TODO - check if this is an accurate and consistent method of classifying global and local effects
		inline constexpr bool is_global() const {
			return excludes_targets(targets, target_t::PROVINCE);
		}
		inline constexpr bool is_local() const {
			return !is_global();
		}
	};

	template<> struct enable_bitfield<ModifierEffect::target_t> : std::true_type {};

	inline constexpr bool ModifierEffect::excludes_targets(target_t targets, target_t excluded_targets) {
		using enum target_t;

		return (targets & excluded_targets) == NO_TARGETS;
	}
}
