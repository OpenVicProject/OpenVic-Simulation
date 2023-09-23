#include "Good.hpp"

#include <cassert>
#include <map>
#include <string_view>
#include "dataloader/NodeTools.hpp"
#include "openvic-dataloader/v2script/AbstractSyntaxTree.hpp"
#include "types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GoodCategory::GoodCategory(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Good::Good(const std::string_view new_identifier, colour_t new_colour, GoodCategory const& new_category, price_t new_base_price,
	bool new_available_from_start, bool new_tradeable, bool new_money, bool new_overseas_penalty)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false },
	  category { new_category },
	  base_price { new_base_price },
	  available_from_start { new_available_from_start },
	  tradeable { new_tradeable },
	  money { new_money },
	  overseas_penalty { new_overseas_penalty } {
	assert(base_price > NULL_PRICE);
}

GoodCategory const& Good::get_category() const {
	return category;
}

Good::price_t Good::get_base_price() const {
	return base_price;
}

Good::price_t Good::get_price() const {
	return price;
}

bool Good::get_available_from_start() const {
	return available_from_start;
}

bool Good::get_available() const {
	return available;
}

bool Good::get_tradeable() const {
	return tradeable;
}

bool Good::get_money() const {
	return money;
}

bool Good::get_overseas_penalty() {
	return overseas_penalty;
}

void Good::reset_to_defaults() {
	available = available_from_start;
	price = base_price;
}

GoodManager::GoodManager() : good_categories { "good categories" }, goods { "goods" } {}

bool GoodManager::add_good_category(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid good category identifier - empty!");
		return false;
	}
	return good_categories.add_item({ identifier });
}

bool GoodManager::add_good(const std::string_view identifier, colour_t colour, GoodCategory const* category,
	Good::price_t base_price, bool available_from_start, bool tradeable, bool money, bool overseas_penalty) {
	if (identifier.empty()) {
		Logger::error("Invalid good identifier - empty!");
		return false;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid good colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	if (category == nullptr) {
		Logger::error("Invalid good category for ", identifier, ": null");
		return false;
	}
	if (base_price <= Good::NULL_PRICE) {
		Logger::error("Invalid base price for ", identifier, ": ", base_price);
		return false;
	}
	return goods.add_item({ identifier, colour, *category, base_price, available_from_start, tradeable, money, overseas_penalty });
}

void GoodManager::reset_to_defaults() {
	for (Good& good : goods.get_items())
		good.reset_to_defaults();
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
	goods.reserve(goods.size() + total_expected_goods);
	ret &= expect_dictionary(
		[this](std::string_view good_category_key, ast::NodeCPtr good_category_value) -> bool {
			GoodCategory const* good_category = get_good_category_by_identifier(good_category_key);

			return expect_dictionary(
				[this, good_category](std::string_view key, ast::NodeCPtr value) -> bool {
					colour_t colour = NULL_COLOUR;
					Good::price_t base_price;
					bool available_from_start, tradeable = true;
					bool money, overseas_penalty = false;

					bool ret = expect_dictionary_keys(
						"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
						"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_price)),
						"available_from_start", ZERO_OR_ONE, expect_bool(assign_variable_callback(available_from_start)),
						"tradeable", ZERO_OR_ONE, expect_bool(assign_variable_callback(tradeable)),
						"money", ZERO_OR_ONE, expect_bool(assign_variable_callback(money)),
						"overseas_penalty", ZERO_OR_ONE, expect_bool(assign_variable_callback(overseas_penalty))
					)(value);
					ret &= add_good(key, colour, good_category, base_price, available_from_start, tradeable, money, overseas_penalty);
					return ret;
				}
			)(good_category_value);
		}
	)(root);
	lock_goods();
	return ret;
}

node_callback_t GoodManager::expect_goods_map(callback_t<std::map<Good const*, fixed_point_t>> cb) {
	return [this, cb](ast::NodeCPtr node) -> bool {
		std::map<Good const*, fixed_point_t> goods_map;
		bool res = expect_good_dictionary([&goods_map](const Good & key, ast::NodeCPtr value) -> bool {
			fixed_point_t good_value;
			bool res = expect_fixed_point((assign_variable_callback(good_value)))(value);
			goods_map.emplace(&key, good_value);
			return res;
		})(node); 
		res &= cb(goods_map);
		return res;
	};
}