#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include <optional>
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/core/container/TypedSpan.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

GoodCategory good_category {"test_good_category"};

const fixed_point_t base_price = 16;
constexpr bool is_not_available_from_start = false;
constexpr bool is_tradeable = true;
constexpr bool is_not_money = false;
constexpr bool does_not_counter_overseas_penalty = false;

GoodDefinition good_definition {
	"test_good",
	colour_rgb_t {},
	good_index_t{0},
	good_category,
	base_price,
	is_not_available_from_start,
	is_tradeable,
	is_not_money,
	does_not_counter_overseas_penalty
};
GameRulesManager game_rules_manager {};

struct Trader {
public:
	std::function<void(BuyResult const&)> buy_callback = [](
		BuyResult const& buy_result
	) -> void {
		FAIL("Did not expect buy callback");
	};
	std::function<void(SellResult const&, memory::vector<fixed_point_t>&)> sell_callback = [](
		SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector
	) -> void {
		FAIL("Did not expect sell callback");
	};

	static void after_buy(void* actor, BuyResult const& buy_result) {
	}
	static void after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) {
	}
};

TEST_CASE("GoodMarket no trading when good isn't available", "[GoodMarket]") {
	GoodMarket good_market { game_rules_manager, good_definition };
	const std::optional<country_index_t> country_index_optional = std::nullopt;

	Trader buyer {
		.buy_callback=[](BuyResult const& buy_result) -> void {
			CHECK(buy_result.good_definition == good_definition);
			CHECK(buy_result.quantity_bought == 0);
			CHECK(buy_result.money_spent_total == 0);
			CHECK(buy_result.money_spent_on_imports == 0);
		}
	};
	const fixed_point_t quantity_to_buy = 1;
	const fixed_point_t money_to_spend = quantity_to_buy * good_market.get_max_next_price();
	good_market.add_buy_up_to_order({
		country_index_optional,
		quantity_to_buy,
		money_to_spend,
		&buyer,
		Trader::after_buy
	});

	Trader seller {
		.sell_callback=[](SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) -> void {
			CHECK(sell_result.good_definition == good_definition);
			CHECK(sell_result.quantity_sold == 0);
			CHECK(sell_result.money_gained == 0);
		}
	};
	const fixed_point_t quantity_to_sell = 1;
	good_market.add_market_sell_order({
		country_index_optional,
		quantity_to_sell,
		&seller,
		Trader::after_sell
	});

	TypedSpan<country_index_t, fixed_point_t> reusable_country_map_0 {};
	TypedSpan<country_index_t, fixed_point_t> reusable_country_map_1 {};
	std::array<memory::vector<fixed_point_t>, GoodMarket::VECTORS_FOR_EXECUTE_ORDERS> reusable_vectors;
	good_market.execute_orders(
		reusable_country_map_0,
		reusable_country_map_1,
		reusable_vectors
	);

	CHECK(good_market.get_price() == base_price);
	CHECK(good_market.get_price_change_yesterday() == 0);
}