#include "StaticModifierCache.hpp"

#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using enum Modifier::modifier_type_t;

StaticModifierCache::StaticModifierCache() {}

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

	#define SET_COUNTRY_MODIFIER(PROP, ...) set_static_country_modifier(PROP);

	// Country modifiers
	COUNTRY_DIFFICULTY_MODIFIER_LIST(SET_COUNTRY_MODIFIER, SET_COUNTRY_MODIFIER)

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

	COUNTRY_MODIFIER_LIST(SET_COUNTRY_MODIFIER, SET_COUNTRY_MODIFIER)

	#undef SET_COUNTRY_MODIFIER

	// Country Event modifiers
	static constexpr IconModifier::icon_t default_icon = 0;
	static constexpr std::string_view bad_debtor_id = "bad_debter"; // paradox typo
	static constexpr std::string_view in_bankruptcy_id = "in_bankrupcy"; // paradox typo
	static constexpr std::string_view generalised_debt_default_id = "generalised_debt_default";
	set_country_event_modifier(bad_debtor_id, default_icon);
	set_country_event_modifier(in_bankruptcy_id, default_icon);
	set_country_event_modifier(generalised_debt_default_id, default_icon);

	#define SET_PROVINCE_MODIFIER(PROP, ...) set_static_province_modifier(PROP);

	// Province modifiers
	PROVINCE_MODIFIER_LIST(SET_PROVINCE_MODIFIER, SET_PROVINCE_MODIFIER)

	#undef SET_PROVINCE_MODIFIER

	ret &= expect_dictionary_key_map_and_default(key_map, key_value_invalid_callback)(root);

	if (ret) {
		bad_debtor = modifier_manager.get_event_modifier_by_identifier(bad_debtor_id);
		in_bankruptcy = modifier_manager.get_event_modifier_by_identifier(in_bankruptcy_id);
		generalised_debt_default = modifier_manager.get_event_modifier_by_identifier(generalised_debt_default_id);
		total_occupation *= 100;
	}

	return ret;
}
