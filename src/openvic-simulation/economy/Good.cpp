#include "Good.hpp"

#include <cassert>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GoodCategory::GoodCategory(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Good::Good(
	std::string_view new_identifier, colour_t new_colour, index_t new_index, GoodCategory const& new_category,
	price_t new_base_price, bool new_available_from_start, bool new_tradeable, bool new_money, bool new_overseas_penalty
) : HasIdentifierAndColour { new_identifier, new_colour, false }, index { new_index }, category { new_category },
	base_price { new_base_price }, available_from_start { new_available_from_start }, tradeable { new_tradeable },
	money { new_money }, overseas_penalty { new_overseas_penalty } {
	assert(base_price > NULL_PRICE);
}

void Good::reset_to_defaults() {
	available = available_from_start;
	price = base_price;
}

bool GoodManager::add_good_category(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid good category identifier - empty!");
		return false;
	}
	return good_categories.add_item({ identifier });
}

bool GoodManager::add_good(
	std::string_view identifier, colour_t colour, GoodCategory const& category, Good::price_t base_price,
	bool available_from_start, bool tradeable, bool money, bool overseas_penalty
) {
	if (identifier.empty()) {
		Logger::error("Invalid good identifier - empty!");
		return false;
	}
	if (base_price <= Good::NULL_PRICE) {
		Logger::error("Invalid base price for ", identifier, ": ", base_price);
		return false;
	}
	return goods.add_item({
		identifier, colour, get_good_count(), category, base_price, available_from_start,
		tradeable, money, overseas_penalty
	});
}

void GoodManager::reset_to_defaults() {
	for (Good& good : goods.get_items()) {
		good.reset_to_defaults();
	}
}

bool GoodManager::load_goods_file(ast::NodeCPtr root) {
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
	reserve_more_goods(total_expected_goods);
	ret &= expect_good_category_dictionary([this](GoodCategory const& good_category, ast::NodeCPtr good_category_value) -> bool {
		return expect_dictionary([this, &good_category](std::string_view key, ast::NodeCPtr value) -> bool {
			colour_t colour = colour_t::null();
			Good::price_t base_price;
			bool available_from_start = true, tradeable = true;
			bool money = false, overseas_penalty = false;

			bool ret = expect_dictionary_keys(
				"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
				"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_price)),
				"available_from_start", ZERO_OR_ONE, expect_bool(assign_variable_callback(available_from_start)),
				"tradeable", ZERO_OR_ONE, expect_bool(assign_variable_callback(tradeable)),
				"money", ZERO_OR_ONE, expect_bool(assign_variable_callback(money)),
				"overseas_penalty", ZERO_OR_ONE, expect_bool(assign_variable_callback(overseas_penalty))
			)(value);
			ret &= add_good(
				key, colour, good_category, base_price, available_from_start, tradeable, money, overseas_penalty
			);
			return ret;
		})(good_category_value);
	})(root);
	lock_goods();
	return ret;
}

bool GoodManager::generate_modifiers(ModifierManager& modifier_manager) const {
	bool ret = true;

	const auto good_modifier = [this, &modifier_manager, &ret](std::string_view name, bool is_positive_good) -> void {
		ret &= modifier_manager.register_complex_modifier(name);
		for (Good const& good : get_goods()) {
			ret &= modifier_manager.add_modifier_effect(
				ModifierManager::get_flat_identifier(name, good.get_identifier()), is_positive_good
			);
		}
	};

	good_modifier("artisan_goods_input", false);
	good_modifier("artisan_goods_output", true);
	good_modifier("artisan_goods_throughput", true);
	good_modifier("factory_goods_input", false);
	good_modifier("factory_goods_output", true);
	good_modifier("factory_goods_throughput", true);
	good_modifier("rgo_goods_output", true);
	good_modifier("rgo_goods_throughput", true);
	good_modifier("rgo_size", true);

	for (Good const& good : get_goods()) {
		ret &= modifier_manager.add_modifier_effect(good.get_identifier(), true, ModifierEffect::format_t::PERCENTAGE_DECIMAL);
	}

	return ret;
}
