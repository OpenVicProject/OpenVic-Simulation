#pragma once

#include <string>
#include <string_view>

#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct ModifierManager;

	struct ModifierEffect : HasIdentifier {
		friend struct ModifierManager;

		enum class format_t : uint8_t {
            PROPORTION_DECIMAL_0DP,
            PROPORTION_DECIMAL_1DP,
            PROPORTION_DECIMAL_2DP,

            /* A fraction/ratio scaled so that 100 is "full"/"whole" */
            PERCENTAGE_DECIMAL, // 2DP, only used for tax_eff

            /* A continuous quantity, e.g. attack strength */
			RAW_DECIMAL_0DP,
            RAW_DECIMAL_1DP,
            RAW_DECIMAL_2DP,
			RAW_DECIMAL_3DP,

            /* A discrete quantity, e.g. building count limit */
            INT_0DP,
            INT_1DP,            /* A discrete quantity, e.g. building count limit */
		};

		enum class target_t : uint8_t {
			NO_TARGETS  = 0,
			COUNTRY     = 1 << 0,
			PROVINCE    = 1 << 1,
			UNIT        = 1 << 2,
			ALL_TARGETS = (1 << 3) - 1
		};

		static constexpr bool excludes_targets(target_t targets, target_t excluded_target);

		static std::string target_to_string(target_t target);

		static std::string make_default_modifier_effect_localisation_key(std::string_view identifier);

	private:
		/* If true, positive values will be green and negative values will be red.
		 * If false, the colours will be switced.
		 */
		const bool PROPERTY_CUSTOM_PREFIX(positive_good, is);
		const bool PROPERTY_CUSTOM_PREFIX(no_effect, has);
		const format_t PROPERTY(format);
		const target_t PROPERTY(targets);
		std::string PROPERTY(localisation_key);

		// TODO - format/precision, e.g. 80% vs 0.8 vs 0.800, 2 vs 2.0 vs 200%

		ModifierEffect(
			std::string_view new_identifier, bool new_is_positive_good, format_t new_format, target_t new_targets,
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
