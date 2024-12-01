#include "BuyResult.hpp"

using namespace OpenVic;

BuyResult::BuyResult(
    const fixed_point_t new_quantity_bought,
    const fixed_point_t new_money_left
) :
    quantity_bought { new_quantity_bought },
    money_left { new_money_left }
    {}