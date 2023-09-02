#include "Good.hpp"

#include <cassert>

using namespace OpenVic;

GoodCategory::GoodCategory(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Good::Good(const std::string_view new_identifier, colour_t new_colour, GoodCategory const& new_category, price_t new_base_price,
	bool new_default_available, bool new_tradeable, bool new_currency, bool new_overseas_maintenance)
	: HasIdentifierAndColour { new_identifier, new_colour, true },
	  category { new_category },
	  base_price { new_base_price },
	  default_available { new_default_available },
	  tradeable { new_tradeable },
	  currency { new_currency },
	  overseas_maintenance { new_overseas_maintenance } {
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

bool Good::is_default_available() const {
	return default_available;
}

bool Good::is_available() const {
	return available;
}

void Good::reset_to_defaults() {
	available = default_available;
	price = base_price;
}

GoodManager::GoodManager() : good_categories { "good categories" }, goods { "goods" } {}

return_t GoodManager::add_good_category(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid good category identifier - empty!");
		return FAILURE;
	}
	return good_categories.add_item({ identifier });
}

void GoodManager::lock_good_categories() {
	good_categories.lock();
}

GoodCategory const* GoodManager::get_good_category_by_identifier(const std::string_view identifier) const {
	return good_categories.get_item_by_identifier(identifier);
}

return_t GoodManager::add_good(const std::string_view identifier, colour_t colour, GoodCategory const* category,
	Good::price_t base_price, bool default_available, bool tradeable, bool currency, bool overseas_maintenance) {
	if (identifier.empty()) {
		Logger::error("Invalid good identifier - empty!");
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid good colour for ", identifier, ": ", Good::colour_to_hex_string(colour));
		return FAILURE;
	}
	if (category == nullptr) {
		Logger::error("Invalid good category for ", identifier, ": null");
		return FAILURE;
	}
	if (base_price <= Good::NULL_PRICE) {
		Logger::error("Invalid base price for ", identifier, ": ", base_price);
		return FAILURE;
	}
	return goods.add_item({ identifier, colour, *category, base_price, default_available, tradeable, currency, overseas_maintenance });
}

void GoodManager::lock_goods() {
	goods.lock();
}

Good const* GoodManager::get_good_by_index(size_t index) const {
	return goods.get_item_by_index(index);
}

Good const* GoodManager::get_good_by_identifier(const std::string_view identifier) const {
	return goods.get_item_by_identifier(identifier);
}

size_t GoodManager::get_good_count() const {
	return goods.size();
}

std::vector<Good> const& GoodManager::get_goods() const {
	return goods.get_items();
}

void GoodManager::reset_to_defaults() {
	for (Good& good : goods.get_items())
		good.reset_to_defaults();
}

return_t GoodManager::load_good_file(ast::NodeCPtr root) {
    return_t ret = NodeTools::expect_dictionary(root, [this](std::string_view key, ast::NodeCPtr value) -> return_t {
        return add_good_category(key);
    }, true);
    lock_good_categories();
    if (NodeTools::expect_dictionary(root, [this](std::string_view good_category_key,
                                                  ast::NodeCPtr good_category_value) -> return_t {

        GoodCategory const *good_category = get_good_category_by_identifier(good_category_key);

        return NodeTools::expect_dictionary(good_category_value, [this, good_category](std::string_view key,
                                                                                       ast::NodeCPtr value) -> return_t {
            colour_t colour = NULL_COLOUR;
            Good::price_t base_price;
            bool default_available, tradeable = true;
            bool currency, overseas_maintenance = false;

            return_t ret = NodeTools::expect_dictionary_keys(value, {
                    {"color",                {true,  false, [&colour](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_colour(node, [&colour](colour_t val) -> return_t {
                            colour = val;
                            return SUCCESS;
                        });
                    }}},
                    {"cost",                 {true,  false, [&base_price](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_fixed_point(node, [&base_price](Good::price_t val) -> return_t {
                            base_price = val;
                            return SUCCESS;
                        });
                    }}},
                    {"available_from_start", {false, false, [&default_available](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_bool(node, [&default_available](bool val) -> return_t {
                            default_available = val;
                            return SUCCESS;
                        });
                    }}},
                    {"tradeable",            {false, false, [&tradeable](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_bool(node, [&tradeable](bool val) -> return_t {
                            tradeable = val;
                            return SUCCESS;
                        });
                    }}},
                    {"money",                {false, false, [&currency](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_bool(node, [&currency](bool val) -> return_t {
                            currency = val;
                            return SUCCESS;
                        });
                    }}},
                    {"overseas_penalty",     {false, false, [&overseas_maintenance](ast::NodeCPtr node) -> return_t {
                        return NodeTools::expect_bool(node, [&overseas_maintenance](bool val) -> return_t {
                            overseas_maintenance = val;
                            return SUCCESS;
                        });
                    }}},
            });
            if (add_good(key, colour, good_category, base_price, default_available, tradeable, currency,
                         overseas_maintenance) != SUCCESS)
                ret = FAILURE;
            return ret;
        });
    }, true) != SUCCESS)
        ret = FAILURE;
    lock_goods();
    return ret;
}
