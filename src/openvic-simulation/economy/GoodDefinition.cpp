#include "GoodDefinition.hpp"

#include "openvic-simulation/core/string/Utility.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GoodCategory::GoodCategory(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

GoodDefinition::GoodDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	GoodCategory const& new_category,
	fixed_point_t new_base_price,
	bool new_is_available_from_start,
	bool new_is_tradeable,
	bool new_is_money,
	bool new_counters_overseas_penalty
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	category { new_category },
	base_price { new_base_price },
	is_available_from_start { new_is_available_from_start },
	is_tradeable { new_is_tradeable },
	is_money { new_is_money },
	counters_overseas_penalty { new_counters_overseas_penalty } {}

bool GoodDefinitionManager::add_good_category(std::string_view identifier, size_t expected_goods_in_category) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid good category identifier - empty!");
		return false;
	}

	if (good_categories.emplace_item(identifier, identifier)) {
		good_categories.back().good_definitions.reserve(expected_goods_in_category);
		return true;
	} else {
		return false;
	}
}

bool GoodDefinitionManager::add_good_definition(
	std::string_view identifier, colour_t colour, GoodCategory& category, fixed_point_t base_price,
	bool is_available_from_start, bool is_tradeable, bool is_money, bool has_overseas_penalty
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid good identifier - empty!");
		return false;
	}
	if (base_price <= 0) {
		spdlog::error_s("Invalid base price for {}: {}", identifier, base_price);
		return false;
	}

	if (is_tradeable == is_money) {
		spdlog::warn_s(
			"Good {} has tradeable: {} and money: {}. Money goods are never tradeable. All other goods are tradeable. Setting tradeable has no effect.",
			identifier, bool_to_yes_no(is_tradeable), bool_to_yes_no(is_money)
		);
	}

	if (good_definitions.emplace_item(
		identifier,
		identifier, colour, get_good_definition_count(), category, base_price, is_available_from_start,
		is_tradeable, is_money, has_overseas_penalty
	)) {
		category.good_definitions.push_back(&get_back_good_definition());
		return true;
	} else {
		return false;
	}
}

bool GoodDefinitionManager::load_goods_file(ast::NodeCPtr root) {
	size_t total_expected_goods = 0;

	bool ret = expect_dictionary_reserve_length(
		good_categories,
		[this, &total_expected_goods](std::string_view key, ast::NodeCPtr value) -> bool {
			size_t expected_goods_in_category = 0;

			bool ret = expect_length(assign_variable_callback(expected_goods_in_category))(value);

			if (add_good_category(key, expected_goods_in_category)) {
				total_expected_goods += expected_goods_in_category;
			} else {
				ret = false;
			}

			return ret;
		}
	)(root);

	lock_good_categories();
	reserve_more_good_definitions(total_expected_goods);

	ret &= good_categories.expect_item_dictionary(
		[this](GoodCategory& good_category, ast::NodeCPtr good_category_value) -> bool {
			return expect_dictionary(
				[this, &good_category](std::string_view key, ast::NodeCPtr value) -> bool {
					colour_t colour = colour_t::null();
					fixed_point_t base_price;
					bool is_available_from_start = true, is_tradeable = true;
					bool is_money = false, has_overseas_penalty = false;

					bool ret = expect_dictionary_keys(
						"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
						"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_price)),
						"available_from_start", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_available_from_start)),
						"tradeable", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_tradeable)),
						"money", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_money)),
						"overseas_penalty", ZERO_OR_ONE, expect_bool(assign_variable_callback(has_overseas_penalty))
					)(value);

					ret &= add_good_definition(
						key, colour, good_category, base_price, is_available_from_start, is_tradeable, is_money,
						has_overseas_penalty
					);

					return ret;
				}
			)(good_category_value);
		}
	)(root);

	lock_good_definitions();

	return ret;
}

bool GoodDefinitionManager::generate_modifiers(ModifierManager& modifier_manager) const {
	static constexpr bool HAS_NO_EFFECT = true;

	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	IndexedFlatMap<GoodDefinition, ModifierEffectCache::good_effects_t>& good_effects =
		modifier_manager.modifier_effect_cache.good_effects;

	good_effects = std::move(decltype(ModifierEffectCache::good_effects){get_good_definitions()});

	bool ret = true;

	ret &= modifier_manager.register_complex_modifier("rgo_goods_input");
	ret &= modifier_manager.register_complex_modifier("rgo_goods_output");
	ret &= modifier_manager.register_complex_modifier("rgo_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("factory_goods_input");
	ret &= modifier_manager.register_complex_modifier("factory_goods_output");
	ret &= modifier_manager.register_complex_modifier("factory_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("artisan_goods_input");
	ret &= modifier_manager.register_complex_modifier("artisan_goods_output");
	ret &= modifier_manager.register_complex_modifier("artisan_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("rgo_size");

	for (GoodDefinition const& good : get_good_definitions()) {
		const std::string_view good_identifier = good.get_identifier();
		ModifierEffectCache::good_effects_t& this_good_effects = good_effects.at(good);

		const auto good_modifier = [&modifier_manager, &ret, &good_identifier](
			ModifierEffect const*& effect_cache, std::string_view name, ModifierEffect::format_t format,
			std::string_view localisation_key, bool has_no_effect = false
		) -> void {
			ret &= modifier_manager.register_technology_modifier_effect(
				effect_cache, ModifierManager::get_flat_identifier(name, good_identifier),
				format, localisation_key, has_no_effect
			);
		};

		const auto make_production_localisation_suffix = [&good](
			std::string_view localisation_suffix
		) -> memory::string {
			return memory::fmt::format("${}$ ${}$", good, localisation_suffix);
		};

		good_modifier(
			this_good_effects.rgo_goods_input, "rgo_goods_input", FORMAT_x100_1DP_PC_NEG,
			make_production_localisation_suffix("TECH_INPUT"), HAS_NO_EFFECT
		);
		good_modifier(
			this_good_effects.rgo_goods_output, "rgo_goods_output", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_OUTPUT")
		);
		good_modifier(
			this_good_effects.rgo_goods_throughput, "rgo_goods_throughput", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_THROUGHPUT")
		);
		good_modifier(
			this_good_effects.factory_goods_input, "factory_goods_input", FORMAT_x100_1DP_PC_NEG,
			make_production_localisation_suffix("TECH_INPUT")
		);
		good_modifier(
			this_good_effects.factory_goods_output, "factory_goods_output", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_OUTPUT")
		);
		good_modifier(
			this_good_effects.factory_goods_throughput, "factory_goods_throughput", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_THROUGHPUT")
		);
		good_modifier(
			this_good_effects.artisan_goods_input, "artisan_goods_input", FORMAT_x100_1DP_PC_NEG,
			make_production_localisation_suffix("TECH_INPUT"), HAS_NO_EFFECT
		);
		good_modifier(
			this_good_effects.artisan_goods_output, "artisan_goods_output", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_OUTPUT"), HAS_NO_EFFECT
		);
		good_modifier(
			this_good_effects.artisan_goods_throughput, "artisan_goods_throughput", FORMAT_x100_1DP_PC_POS,
			make_production_localisation_suffix("TECH_THROUGHPUT"), HAS_NO_EFFECT
		);
		good_modifier(
			this_good_effects.rgo_size, "rgo_size", FORMAT_x100_2DP_PC_POS,
			memory::fmt::format("{}_RGO_SIZE", good)
		);
	}

	return ret;
}
