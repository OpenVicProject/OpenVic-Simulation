#include "NationalFocusManager.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/GoodDefinitionManager.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/politics/IdeologyManager.hpp"
#include "openvic-simulation/population/PopManager.hpp"
#include "openvic-simulation/core/FormatValidate.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

inline bool NationalFocusManager::add_national_focus_group(std::string_view identifier) {
	if (identifier.empty()) {
		spdlog::error_s("No identifier for national focus group!");
		return false;
	}

	return national_focus_groups.emplace_item(identifier, identifier);
}

inline bool NationalFocusManager::add_national_focus(
	std::string_view identifier,
	NationalFocusGroup const& group,
	uint8_t icon,
	bool has_flashpoint,
	fixed_point_t flashpoint_tension,
	bool own_provinces,
	bool outliner_show_as_percent,
	ModifierValue&& modifiers,
	Ideology const* loyalty_ideology,
	fixed_point_t loyalty_value,
	fixed_point_t encourage_railroads,
	fixed_point_map_t<GoodDefinition const*>&& encourage_goods,
	fixed_point_map_t<PopType const*>&& encourage_pop_types,
	ConditionScript&& limit
) {
	spdlog::scope scope { fmt::format("national focus {}", identifier) };
	if (identifier.empty()) {
		spdlog::error_s("No identifier for national focus!");
		return false;
	}

	if (icon < 1) {
		spdlog::error_s("Invalid icon {} for national focus {}", icon, identifier);
		return false;
	}

	if ((loyalty_ideology == nullptr) != (loyalty_value == 0)) {
		spdlog::warn_s(
			"Party loyalty incorrectly defined for national focus {}: ideology = {}, value = {}",
			identifier, ovfmt::validate(loyalty_ideology), loyalty_value
		);
	}

	return national_foci.emplace_item(
		identifier,
		identifier, group, icon, has_flashpoint, flashpoint_tension, own_provinces, outliner_show_as_percent,
		std::move(modifiers), loyalty_ideology, loyalty_value, encourage_railroads, std::move(encourage_goods),
		std::move(encourage_pop_types), std::move(limit)
	);
}

bool NationalFocusManager::load_national_foci_file(
	PopManager const& pop_manager, IdeologyManager const& ideology_manager,
	GoodDefinitionManager const& good_definition_manager, ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root
) {
	spdlog::scope scope { "common/national_focus.txt" };
	size_t expected_national_foci = 0;

	bool ret = expect_dictionary_reserve_length(
		national_focus_groups,
		[this, &expected_national_foci](std::string_view identifier, ovdl::v2script::ast::Node const* node) -> bool {
			return expect_length(add_variable_callback(expected_national_foci))(node) & add_national_focus_group(identifier);
		}
	)(root);

	lock_national_focus_groups();

	reserve_more_national_foci(expected_national_foci);

	ret &= expect_national_focus_group_dictionary(
		[this, &pop_manager, &ideology_manager, &good_definition_manager, &modifier_manager](
			NationalFocusGroup const& group, ovdl::v2script::ast::Node const* group_node
		) -> bool {
			return expect_dictionary(
				[this, &group, &pop_manager, &ideology_manager, &good_definition_manager, &modifier_manager](
					std::string_view identifier, ovdl::v2script::ast::Node const* node
				) -> bool {
					spdlog::scope scope { fmt::format("national focus {}", identifier) };
					using enum scope_type_t;

					uint8_t icon = 0;
					bool has_flashpoint = false, own_provinces = true, outliner_show_as_percent = false;
					fixed_point_t flashpoint_tension = 0;
					ModifierValue modifiers;
					Ideology const* loyalty_ideology = nullptr;
					fixed_point_t loyalty_value = 0;
					fixed_point_t encourage_railroads = 0;
					fixed_point_map_t<GoodDefinition const*> encourage_goods;
					fixed_point_map_t<PopType const*> encourage_pop_types;
					ConditionScript limit {
						PROVINCE | COUNTRY, PROVINCE | COUNTRY, NO_SCOPE
					};

					auto expect_base_province_modifier_cb = modifier_manager.expect_base_province_modifier(modifiers);
					bool ret = NodeTools::expect_dictionary_keys_and_default(
						[&good_definition_manager, &encourage_goods, &pop_manager, &encourage_pop_types, &expect_base_province_modifier_cb](
							std::string_view key, ovdl::v2script::ast::Node const* value
						) mutable -> bool {
							GoodDefinition const* good = good_definition_manager.get_good_definition_by_identifier(key);
							if (good != nullptr) {
								return expect_fixed_point(map_callback(encourage_goods, good))(value);
							}

							PopType const* pop_type = pop_manager.get_pop_type_by_identifier(key);
							if (pop_type != nullptr) {
								return expect_fixed_point(map_callback(encourage_pop_types, pop_type))(value);
							}

							return expect_base_province_modifier_cb(key, value) || key_value_invalid_callback(key, value);
						},
						"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
						"ideology", ZERO_OR_ONE,
							ideology_manager.expect_ideology_identifier(assign_variable_callback_pointer(loyalty_ideology)),
						"loyalty_value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(loyalty_value)),
						"limit", ZERO_OR_ONE, limit.expect_script(),
						"has_flashpoint", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_flashpoint)),
						"flashpoint_tension", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(flashpoint_tension)),
						"own_provinces", ZERO_OR_ONE, expect_bool(assign_variable_callback(own_provinces)),
						"outliner_show_as_percent", ZERO_OR_ONE,
							expect_bool(assign_variable_callback(outliner_show_as_percent)),
						"railroads", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(encourage_railroads))
					)(node);

					ret &= add_national_focus(
						identifier, group, icon, has_flashpoint, flashpoint_tension, own_provinces, outliner_show_as_percent,
						std::move(modifiers), loyalty_ideology, loyalty_value, encourage_railroads,
						std::move(encourage_goods), std::move(encourage_pop_types), std::move(limit)
					);

					return ret;
				}
			)(group_node);
		}
	)(root);

	lock_national_foci();

	return ret;
}

bool NationalFocusManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (NationalFocus& national_focus : national_foci.get_items()) {
		ret &= national_focus.parse_scripts(definition_manager);
	}

	return ret;
}
