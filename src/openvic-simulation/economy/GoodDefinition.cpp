#include "GoodDefinition.hpp"

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

bool GoodDefinitionManager::add_good_category(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid good category identifier - empty!");
		return false;
	}
	return good_categories.add_item({ identifier });
}

bool GoodDefinitionManager::add_good_definition(
	std::string_view identifier, colour_t colour, GoodCategory const& category, fixed_point_t base_price,
	bool is_available_from_start, bool is_tradeable, bool is_money, bool has_overseas_penalty
) {
	if (identifier.empty()) {
		Logger::error("Invalid good identifier - empty!");
		return false;
	}
	if (base_price <= 0) {
		Logger::error("Invalid base price for ", identifier, ": ", base_price);
		return false;
	}
	return good_definitions.add_item({
		identifier, colour, get_good_definition_count(), category, base_price, is_available_from_start,
		is_tradeable, is_money, has_overseas_penalty
	});
}

bool GoodDefinitionManager::load_goods_file(ast::NodeCPtr root) {
	size_t total_expected_goods = 0;
	bool ret = expect_dictionary_reserve_length(
		good_categories,
		[this, &total_expected_goods](std::string_view key, ast::NodeCPtr value) -> bool {
			bool ret = expect_length(add_variable_callback(total_expected_goods))(value);
			ret &= add_good_category(key);
			return ret;
		}
	)(root);
	lock_good_categories();
	reserve_more_good_definitions(total_expected_goods);
	ret &= expect_good_category_dictionary([this](GoodCategory const& good_category, ast::NodeCPtr good_category_value) -> bool {
		return expect_dictionary([this, &good_category](std::string_view key, ast::NodeCPtr value) -> bool {
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
				key, colour, good_category, base_price, is_available_from_start, is_tradeable, is_money, has_overseas_penalty
			);
			return ret;
		})(good_category_value);
	})(root);
	lock_good_definitions();
	return ret;
}

bool GoodDefinitionManager::generate_modifiers(ModifierManager& modifier_manager) const {
	constexpr bool has_no_effect = true;
	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	IndexedMap<GoodDefinition, ModifierEffectCache::good_effects_t>& good_effects =
		modifier_manager.modifier_effect_cache.good_effects;

	good_effects.set_keys(&get_good_definitions());

	bool ret = true;

	ret &= modifier_manager.register_complex_modifier("artisan_goods_input");
	ret &= modifier_manager.register_complex_modifier("artisan_goods_output");
	ret &= modifier_manager.register_complex_modifier("artisan_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("factory_goods_input");
	ret &= modifier_manager.register_complex_modifier("factory_goods_output");
	ret &= modifier_manager.register_complex_modifier("factory_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("rgo_goods_output");
	ret &= modifier_manager.register_complex_modifier("rgo_goods_throughput");
	ret &= modifier_manager.register_complex_modifier("rgo_size");

	for (GoodDefinition const& good : get_good_definitions()) {
		const std::string_view good_identifier = good.get_identifier();
		ModifierEffectCache::good_effects_t& this_good_effects = good_effects[good];

		const auto good_modifier = [&modifier_manager, &ret, &good_identifier](
			ModifierEffect const*& effect_cache, std::string_view name, bool is_positive_good,
			std::string_view localisation_key, bool has_no_effect = false
		) -> void {
			ret &= modifier_manager.register_technology_modifier_effect(
				effect_cache, ModifierManager::get_flat_identifier(name, good_identifier), is_positive_good,
				PROPORTION_DECIMAL, localisation_key, has_no_effect
			);
		};

		const auto make_production_localisation_suffix = [&good_identifier](
			std::string_view localisation_suffix
		) -> std::string {
			return StringUtils::append_string_views("$", good_identifier, "$ $", localisation_suffix, "$");
		};

		good_modifier(
			this_good_effects.artisan_goods_input, "artisan_goods_input", false,
			make_production_localisation_suffix("TECH_INPUT"), has_no_effect
		);
		good_modifier(
			this_good_effects.artisan_goods_output, "artisan_goods_output", true,
			make_production_localisation_suffix("TECH_OUTPUT"), has_no_effect
		);
		good_modifier(
			this_good_effects.artisan_goods_throughput, "artisan_goods_throughput", true,
			make_production_localisation_suffix("TECH_THROUGHPUT"), has_no_effect
		);
		good_modifier(
			this_good_effects.factory_goods_input, "factory_goods_input", false,
			make_production_localisation_suffix("TECH_INPUT")
		);
		good_modifier(
			this_good_effects.factory_goods_output, "factory_goods_output", true,
			make_production_localisation_suffix("TECH_OUTPUT")
		);
		good_modifier(
			this_good_effects.factory_goods_throughput, "factory_goods_throughput", true,
			make_production_localisation_suffix("TECH_THROUGHPUT")
		);
		good_modifier(
			this_good_effects.rgo_goods_output, "rgo_goods_output", true,
			make_production_localisation_suffix("TECH_OUTPUT")
		);
		good_modifier(
			this_good_effects.rgo_goods_throughput, "rgo_goods_throughput", true,
			make_production_localisation_suffix("TECH_THROUGHPUT")
		);
		good_modifier(
			this_good_effects.rgo_size, "rgo_size", true,
			StringUtils::append_string_views(good_identifier, "_RGO_SIZE")
		);
	}

	return ret;
}
