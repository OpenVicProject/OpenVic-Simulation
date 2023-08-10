#include "Good.hpp"

#include <cassert>

using namespace OpenVic;

Good::Good(std::string const& new_identifier, std::string const& new_category, colour_t new_colour, price_t new_base_price,
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

std::string const& Good::get_category() const {
	return category;
}

price_t Good::get_base_price() const {
	return base_price;
}

price_t Good::get_price() const {
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

GoodManager::GoodManager() : goods { "goods" } {}

return_t GoodManager::add_good(std::string const& identifier, std::string const& category, colour_t colour,
	price_t base_price, bool default_available, bool tradeable, bool currency, bool overseas_maintenance) {
	if (identifier.empty()) {
		Logger::error("Invalid good identifier - empty!");
		return FAILURE;
	}
	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid good colour for ", identifier, ": ", Good::colour_to_hex_string(colour));
		return FAILURE;
	}
	if (category.empty()) {
		Logger::error("Invalid good category for ", identifier, ": empty!");
		return FAILURE;
	}
	if (base_price <= NULL_PRICE) {
		Logger::error("Invalid base price for ", identifier, ": ", base_price);
		return FAILURE;
	}
	return goods.add_item({ identifier, category, colour, base_price, default_available, tradeable, currency, overseas_maintenance });
}

void GoodManager::lock_goods() {
	goods.lock();
}

void GoodManager::reset_to_defaults() {
	for (Good& good : goods.get_items())
		good.reset_to_defaults();
}

Good const* GoodManager::get_good_by_index(size_t index) const {
	return goods.get_item_by_index(index);
}

size_t GoodManager::get_good_count() const {
	return goods.get_item_count();
}

std::vector<Good> const& GoodManager::get_goods() const {
	return goods.get_items();
}
