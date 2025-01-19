#include "StaticModifierCache.hpp"

#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using enum Modifier::modifier_type_t;

StaticModifierCache::StaticModifierCache()
  : // Country modifiers
	very_easy_player { "very_easy_player", {}, STATIC },
	easy_player { "easy_player", {}, STATIC },
	hard_player { "hard_player", {}, STATIC },
	very_hard_player { "very_hard_player", {}, STATIC },
	very_easy_ai { "very_easy_ai", {}, STATIC },
	easy_ai { "easy_ai", {}, STATIC },
	hard_ai { "hard_ai", {}, STATIC },
	very_hard_ai { "very_hard_ai", {}, STATIC },
	base_modifier { "base_values", {}, STATIC },
	war { "war", {}, STATIC },
	peace { "peace", {}, STATIC },
	disarming { "disarming", {}, STATIC },
	war_exhaustion { "war_exhaustion", {}, STATIC },
	infamy { "badboy", {}, STATIC },
	debt_default_to { "debt_default_to", {}, STATIC },
	great_power { "great_power", {}, STATIC },
	secondary_power { "second_power", {}, STATIC },
	civilised { "civ_nation", {}, STATIC },
	uncivilised { "unciv_nation", {}, STATIC },
	literacy { "average_literacy", {}, STATIC },
	plurality { "plurality", {}, STATIC },
	total_occupation { "total_occupation", {}, STATIC },
	total_blockaded { "total_blockaded", {}, STATIC },
	in_bankruptcy { nullptr },
	bad_debtor { nullptr },
	generalised_debt_default { nullptr },
	// Province modifiers
	overseas { "overseas", {}, STATIC },
	coastal { "coastal", {}, STATIC },
	non_coastal { "non_coastal", {}, STATIC },
	coastal_sea { "coastal_sea", {}, STATIC },
	sea_zone { "sea_zone", {}, STATIC },
	land_province { "land_province", {}, STATIC },
	blockaded { "blockaded", {}, STATIC },
	no_adjacent_controlled { "no_adjacent_controlled", {}, STATIC },
	core { "core", {}, STATIC },
	has_siege { "has_siege", {}, STATIC },
	occupied { "occupied", {}, STATIC },
	nationalism { "nationalism", {}, STATIC },
	infrastructure { "infrastructure", {}, STATIC } {}

bool StaticModifierCache::load_static_modifiers(ModifierManager& modifier_manager, const ast::NodeCPtr root) {
	bool ret = true;
	key_map_t key_map {};

	const auto set_static_country_modifier = [&key_map, &modifier_manager, &ret](
		Modifier& modifier
	) -> void {
		ret &= add_key_map_entry(
			key_map, modifier.get_identifier(), ONE_EXACTLY,
			expect_dictionary(modifier_manager.expect_base_country_modifier(modifier))
		);
	};

	const auto set_country_event_modifier = [&key_map, &modifier_manager, &ret](
		const std::string_view identifier, const IconModifier::icon_t icon
	) -> void {
		ret &= add_key_map_entry(
			key_map, identifier, ONE_EXACTLY,
			[&modifier_manager, identifier, icon](const ast::NodeCPtr value) -> bool {
				if (modifier_manager.get_event_modifier_by_identifier(identifier) != nullptr) {
					return true; //an event modifier overrides a static modifier with the same identifier.
				}

				ModifierValue modifier_value {};
				bool has_parsed_modifier = expect_dictionary(modifier_manager.expect_base_country_modifier(modifier_value))(value);
				has_parsed_modifier &= modifier_manager.add_event_modifier(identifier, std::move(modifier_value), icon, STATIC);
				return has_parsed_modifier;
			}
		);
	};

	const auto set_static_province_modifier = [&key_map, &modifier_manager, &ret](
		Modifier& modifier
	) -> void {
		ret &= add_key_map_entry(
			key_map, modifier.get_identifier(), ONE_EXACTLY,
			expect_dictionary(modifier_manager.expect_base_province_modifier(modifier))
		);
	};

	// Country modifiers
	set_static_country_modifier(very_easy_player);
	set_static_country_modifier(easy_player);
	set_static_country_modifier(hard_player);
	set_static_country_modifier(very_hard_player);
	set_static_country_modifier(very_easy_ai);
	set_static_country_modifier(easy_ai);
	set_static_country_modifier(hard_ai);
	set_static_country_modifier(very_hard_ai);

	ret &= add_key_map_entry(
		key_map, base_modifier.get_identifier(), ONE_EXACTLY,
		expect_dictionary_keys_and_default(
			modifier_manager.expect_base_country_modifier(base_modifier),
			"supply_limit", ZERO_OR_ONE, [this, &modifier_manager](const ast::NodeCPtr value) -> bool {
				return modifier_manager._add_modifier_cb(
					base_modifier,
					modifier_manager.get_modifier_effect_cache().get_supply_limit_global_base(),
					value
				);
			}
		)
	);

	set_static_country_modifier(war);
	set_static_country_modifier(peace);
	set_static_country_modifier(disarming);
	set_static_country_modifier(war_exhaustion);
	set_static_country_modifier(infamy);
	set_static_country_modifier(debt_default_to);
	set_static_country_modifier(great_power);
	set_static_country_modifier(secondary_power);
	set_static_country_modifier(civilised);
	set_static_country_modifier(uncivilised);
	set_static_country_modifier(literacy);
	set_static_country_modifier(plurality);
	set_static_country_modifier(total_occupation);
	set_static_country_modifier(total_blockaded);

	// Country Event modifiers
	static constexpr IconModifier::icon_t default_icon = 0;
	static constexpr std::string_view bad_debtor_id = "bad_debter"; //paradox typo
	static constexpr std::string_view in_bankruptcy_id = "in_bankrupcy"; //paradox typo
	static constexpr std::string_view generalised_debt_default_id = "generalised_debt_default";
	set_country_event_modifier(bad_debtor_id, default_icon);
	set_country_event_modifier(in_bankruptcy_id, default_icon);
	set_country_event_modifier(generalised_debt_default_id, default_icon);

	// Province modifiers
	set_static_province_modifier(overseas);
	set_static_province_modifier(coastal);
	set_static_province_modifier(non_coastal);
	set_static_province_modifier(coastal_sea);
	set_static_province_modifier(sea_zone);
	set_static_province_modifier(land_province);
	set_static_province_modifier(blockaded);
	set_static_province_modifier(no_adjacent_controlled);
	set_static_province_modifier(core);
	set_static_province_modifier(has_siege);
	set_static_province_modifier(occupied);
	set_static_province_modifier(nationalism);
	set_static_province_modifier(infrastructure);

	ret &= expect_dictionary_key_map_and_default(key_map, key_value_invalid_callback)(root);

	if (ret) {
		bad_debtor = modifier_manager.get_event_modifier_by_identifier(bad_debtor_id);
		in_bankruptcy = modifier_manager.get_event_modifier_by_identifier(in_bankruptcy_id);
		generalised_debt_default = modifier_manager.get_event_modifier_by_identifier(generalised_debt_default_id);
		total_occupation *= 100;
	}

	return ret;
}
