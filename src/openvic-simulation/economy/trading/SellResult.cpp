#include "SellResult.hpp"

using namespace OpenVic;

SellResult::SellResult(
    const fixed_point_t new_quantity_sold,
    const fixed_point_t new_money_gained
) :
    quantity_sold { new_quantity_sold },
    money_gained { new_money_gained }
    {}