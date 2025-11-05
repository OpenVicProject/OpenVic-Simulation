#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/core/Property.hpp"

#define COUNTRY_DIFFICULTY_MODIFIER_LIST(F, ID_F) \
	F(very_easy_player) \
	F(easy_player) \
	F(hard_player) \
	F(very_hard_player) \
	F(very_easy_ai) \
	F(easy_ai) \
	F(hard_ai) \
	F(very_hard_ai) \

#define COUNTRY_MODIFIER_LIST(F, ID_F) \
	F(war) \
	F(peace) \
	F(disarming) \
	F(war_exhaustion) \
	ID_F(infamy, badboy) \
	F(debt_default_to) \
	F(great_power) \
	ID_F(secondary_power, second_power) \
	ID_F(civilised, civ_nation) \
	ID_F(uncivilised, unciv_nation) \
	ID_F(literacy, average_literacy) \
	F(plurality) \
	F(total_occupation) \
	F(total_blockaded)

// Province modifiers
#define PROVINCE_MODIFIER_LIST(F, ID_F) \
	F(overseas) \
	F(coastal) \
	F(non_coastal) \
	F(coastal_sea) \
	F(sea_zone) \
	F(land_province) \
	F(blockaded) \
	F(no_adjacent_controlled) \
	F(core) \
	F(has_siege) \
	F(occupied) \
	F(nationalism) \
	F(infrastructure)

namespace OpenVic {
	struct ModifierManager;

	struct StaticModifierCache {
		friend struct ModifierManager;

	private:
		#define DEFINE_PROPERTY_ID(PROP, ID) Modifier PROPERTY(PROP, { #ID, {}, Modifier::modifier_type_t::STATIC });
		#define DEFINE_PROPERTY(PROP) DEFINE_PROPERTY_ID(PROP, PROP)

		COUNTRY_DIFFICULTY_MODIFIER_LIST(DEFINE_PROPERTY, DEFINE_PROPERTY_ID)
		COUNTRY_MODIFIER_LIST(DEFINE_PROPERTY, DEFINE_PROPERTY_ID)
		PROVINCE_MODIFIER_LIST(DEFINE_PROPERTY, DEFINE_PROPERTY_ID)

		#undef DEFINE_PROPERTY
		#undef DEFINE_PROPERTY_NAME

		Modifier PROPERTY(base_modifier, { "base_values", {}, Modifier::modifier_type_t::STATIC });

		// Country event modifiers
		Modifier const* PROPERTY(in_bankruptcy, nullptr);
		Modifier const* PROPERTY(bad_debtor, nullptr);
		Modifier const* PROPERTY(generalised_debt_default, nullptr); //possibly, as it's used to trigger gunboat CB

		StaticModifierCache();

		bool load_static_modifiers(ModifierManager& modifier_manager, const ast::NodeCPtr root);

	public:
		StaticModifierCache(StaticModifierCache&&) = default;
	};
}
