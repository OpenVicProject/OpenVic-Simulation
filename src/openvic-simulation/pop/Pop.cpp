#include <utility>

#define KEEP_DO_FOR_ALL_TYPES_OF_INCOME
#define KEEP_DO_FOR_ALL_TYPES_OF_EXPENSES
#define KEEP_DO_FOR_ALL_NEED_CATEGORIES
#include "Pop.hpp"
#undef KEEP_DO_FOR_ALL_TYPES_OF_INCOME
#undef KEEP_DO_FOR_ALL_TYPES_OF_EXPENSES
#undef KEEP_DO_FOR_ALL_NEED_CATEGORIES

#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerFactoryPattern.hpp"
#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/pop/PopValuesFromProvince.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

PopBase::PopBase(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { &new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, militancy { new_militancy },
	consciousness { new_consciousness }, rebel_type { new_rebel_type } {}

Pop::Pop(
	PopBase const& pop_base,
	decltype(ideology_distribution)::keys_type const& ideology_keys,
	MarketInstance& new_market_instance,
	ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
)
  : PopBase { pop_base },
	market_instance { new_market_instance },
	artisanal_producer_nullable {
		type->get_is_artisan()
			? artisanal_producer_factory_pattern.CreateNewArtisanalProducer()
			: nullptr
	},
	ideology_distribution { &ideology_keys },
	vote_distribution { nullptr } {
		reserve_needs_fulfilled_goods();
	}

void Pop::setup_pop_test_values(IssueManager const& issue_manager) {
	/* Returns +/- range% of size. */
	const auto test_size = [this](int32_t range) -> pop_size_t {
		return size * ((rand() % (2 * range + 1)) - range) / 100;
	};

	num_grown = test_size(5);
	num_promoted = test_size(1);
	num_demoted = test_size(1);
	num_migrated_internal = test_size(3);
	num_migrated_external = test_size(1);
	num_migrated_colonial = test_size(2);

	total_change =
		num_grown + num_promoted + num_demoted + num_migrated_internal + num_migrated_external + num_migrated_colonial;

	/* Generates a number between 0 and max (inclusive) and sets map[&key] to it if it's at least min. */
	auto test_weight =
		[]<typename T, typename U>(T& map, U const& key, int32_t min, int32_t max) -> void {
			const int32_t value = rand() % (max + 1);
			if (value >= min) {
				if constexpr (utility::is_specialization_of_v<T, IndexedMap>) {
					map[key] = value;
				} else {
					map.emplace(&key, value);
				}
			}
		};

	/* All entries equally weighted for testing. */
	ideology_distribution.clear();
	for (Ideology const& ideology : *ideology_distribution.get_keys()) {
		test_weight(ideology_distribution, ideology, 1, 5);
	}
	ideology_distribution.rescale(size);

	issue_distribution.clear();
	for (Issue const& issue : issue_manager.get_issues()) {
		test_weight(issue_distribution, issue, 3, 6);
	}
	for (Reform const& reform : issue_manager.get_reforms()) {
		if (!reform.get_reform_group().is_uncivilised()) {
			test_weight(issue_distribution, reform, 3, 6);
		}
	}
	rescale_fixed_point_map(issue_distribution, size);

	if (vote_distribution.has_keys()) {
		vote_distribution.clear();
		for (CountryParty const& party : *vote_distribution.get_keys()) {
			test_weight(vote_distribution, party, 4, 10);
		}
		vote_distribution.rescale(size);
	}

	/* Returns a fixed point between 0 and max. */
	const auto test_range = [](fixed_point_t max = 1) -> fixed_point_t {
		return (rand() % 256) * max / 256;
	};

	cash = test_range(20);
	income = test_range(5);
	expenses = test_range(5);
	savings = test_range(15);
	life_needs_acquired_quantity = test_range();
	everyday_needs_acquired_quantity = test_range();
	luxury_needs_acquired_quantity = test_range();
	life_needs_desired_quantity = everyday_needs_desired_quantity = luxury_needs_desired_quantity = fixed_point_t::_1();
}

bool Pop::convert_to_equivalent() {
	PopType const* const equivalent = get_type()->get_equivalent();
	if (equivalent == nullptr) {
		Logger::error("Tried to convert pop of type ", get_type()->get_identifier(), " to equivalent, but there is no equivalent.");
		return false;
	}

	type = equivalent;
	reserve_needs_fulfilled_goods();
	return true;
}

void Pop::set_location(ProvinceInstance& new_location) {
	if (location != &new_location) {
		location = &new_location;

		update_location_based_attributes();
	}
}

void Pop::update_location_based_attributes() {
	if (location != nullptr) {
		CountryInstance const* owner = location->get_owner();

		if (owner != nullptr) {
			vote_distribution.set_keys(&owner->get_country_definition()->get_parties());

			// TODO - calculate vote distribution

			return;
		}
	}

	vote_distribution.set_keys(nullptr);
}

fixed_point_t Pop::get_ideology_support(Ideology const& ideology) const {
	return ideology_distribution[ideology];
}

fixed_point_t Pop::get_issue_support(Issue const& issue) const {
	const decltype(issue_distribution)::const_iterator it = issue_distribution.find(&issue);

	if (it != issue_distribution.end()) {
		return it->second;
	} else {
		return fixed_point_t::_0();
	}
}

fixed_point_t Pop::get_party_support(CountryParty const& party) const {
	if (vote_distribution.has_keys()) {
		return vote_distribution[party];
	} else {
		return fixed_point_t::_0();
	}
}

void Pop::update_gamestate(
	DefineManager const& define_manager, CountryInstance const* owner, const fixed_point_t pop_size_per_regiment_multiplier
) {
	static constexpr fixed_point_t MIN_MILITANCY = fixed_point_t::_0();
	static constexpr fixed_point_t MAX_MILITANCY = fixed_point_t::_10();
	static constexpr fixed_point_t MIN_CONSCIOUSNESS = fixed_point_t::_0();
	static constexpr fixed_point_t MAX_CONSCIOUSNESS = fixed_point_t::_10();
	static constexpr fixed_point_t MIN_LITERACY = fixed_point_t::_0_01();
	static constexpr fixed_point_t MAX_LITERACY = fixed_point_t::_1();

	militancy = std::clamp(militancy, MIN_MILITANCY, MAX_MILITANCY);
	consciousness = std::clamp(consciousness, MIN_CONSCIOUSNESS, MAX_CONSCIOUSNESS);
	literacy = std::clamp(literacy, MIN_LITERACY, MAX_LITERACY);

	if (type->get_can_be_recruited()) {
		MilitaryDefines const& military_defines = define_manager.get_military_defines();

		if (
			size < military_defines.get_min_pop_size_for_regiment() || owner == nullptr ||
			!RegimentType::allowed_cultures_check_culture_in_country(owner->get_allowed_regiment_cultures(), culture, *owner)
		) {
			max_supported_regiments = 0;
		} else {
			max_supported_regiments = (fixed_point_t::parse(size) / (
				fixed_point_t::parse(military_defines.get_pop_size_per_regiment()) * pop_size_per_regiment_multiplier
			)).to_int64_t() + 1;
		}
	}
}

std::stringstream Pop::get_pop_context_text() const {
	std::stringstream pop_context {};
	pop_context << " location: ";
	if (OV_unlikely(location == nullptr)) {
		pop_context << "NULL";
	} else {
		pop_context << location->get_identifier();
	}
	pop_context << " type: " << type << " culture: " << culture << " religion: " << religion << " size: " << size;
	return pop_context;
}

void Pop::reserve_needs_fulfilled_goods() {
	PopType const& type_never_null = *type;
	#define RESERVE_NEEDS(need_category) \
		need_category##_needs_fulfilled_goods.reserve(type_never_null.get_##need_category##_needs().size());

	DO_FOR_ALL_NEED_CATEGORIES(RESERVE_NEEDS)
	#undef RESERVE_NEEDS
}

void Pop::fill_needs_fulfilled_goods_with_false() {
	PopType const& type_never_null = *type;
	#define FILL_WITH_FALSE(need_category) \
		need_category##_needs_fulfilled_goods.clear(); \
		for (auto [good, base_demand] : type_never_null.get_##need_category##_needs()) { \
			need_category##_needs_fulfilled_goods.emplace(good, false); \
		}

	DO_FOR_ALL_NEED_CATEGORIES(FILL_WITH_FALSE)
	#undef FILL_WITH_FALSE
}

void Pop::pay_income_tax(fixed_point_t& income) {
	CountryInstance* const tax_collector_nullable = location->get_country_to_report_economy();
	if (tax_collector_nullable == nullptr) {
		return;
	}
	const fixed_point_t effective_tax_rate = tax_collector_nullable->get_effective_tax_rate_by_strata()[type->get_strata()];
	const fixed_point_t tax = effective_tax_rate * income;
	tax_collector_nullable->report_pop_income_tax(*type, income, tax);
	income -= tax;
}

#define DEFINE_ADD_INCOME_FUNCTIONS(name) \
	void Pop::add_##name(fixed_point_t amount){ \
		if (OV_unlikely(amount == fixed_point_t::_0())) { \
			Logger::warning("Adding ", #name, " of 0 to pop. Context", get_pop_context_text().str()); \
			return; \
		} \
		if (OV_unlikely(amount < fixed_point_t::_0())) { \
			Logger::error("Adding negative ", #name, " of ", amount, " to pop. Context", get_pop_context_text().str()); \
			return; \
		} \
		pay_income_tax(amount); \
		name += amount; \
		income += amount; \
		cash += amount; \
	}

DO_FOR_ALL_TYPES_OF_POP_INCOME(DEFINE_ADD_INCOME_FUNCTIONS)
#undef DEFINE_ADD_INCOME_FUNCTIONS

#define DEFINE_ADD_EXPENSE_FUNCTIONS(name) \
	void Pop::add_##name(const fixed_point_t amount){ \
		if (OV_unlikely(amount == fixed_point_t::_0())) { \
			Logger::warning("Adding ", #name, " of 0 to pop. Context:", get_pop_context_text().str()); \
			return; \
		} \
		name += amount; \
		const fixed_point_t expenses_copy = expenses += amount; \
		if (OV_unlikely(expenses_copy < fixed_point_t::_0())) { \
			Logger::error("Total expenses became negative (", expenses_copy, ") after adding ", #name, " of ", amount, " to pop. Context:", get_pop_context_text().str()); \
		} \
		const fixed_point_t cash_copy = cash -= amount; \
		if (OV_unlikely(cash_copy < fixed_point_t::_0())) { \
			Logger::error("Total cash became negative (", cash_copy, ") after adding ", #name, " of ", amount, " to pop. Context:", get_pop_context_text().str()); \
		} \
	}

DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DEFINE_ADD_EXPENSE_FUNCTIONS)
#undef DEFINE_ADD_EXPENSE_FUNCTIONS

#define DEFINE_NEEDS_FULFILLED(need_category) \
	fixed_point_t Pop::get_##need_category##_needs_fulfilled() const { \
		const fixed_point_t desired_quantity_copy = need_category##_needs_desired_quantity.get_copy_of_value(); \
		if (desired_quantity_copy == fixed_point_t::_0()) { \
			return fixed_point_t::_1(); \
		} \
		return need_category##_needs_acquired_quantity.get_copy_of_value() / desired_quantity_copy; \
	}
DO_FOR_ALL_NEED_CATEGORIES(DEFINE_NEEDS_FULFILLED)
#undef DEFINE_NEEDS_FULFILLED

#define SET_TO_ZERO(name) \
	name = fixed_point_t::_0();

void Pop::allocate_for_needs(
	GoodDefinition::good_definition_map_t const& scaled_needs,
	GoodDefinition::good_definition_map_t& money_to_spend_per_good,
	GoodDefinition::good_definition_map_t& reusable_map_0,
	fixed_point_t& price_inverse_sum,
	fixed_point_t& cash_left_to_spend
) {
	GoodDefinition::good_definition_map_t& money_to_spend_per_good_draft = reusable_map_0;
	fixed_point_t cash_left_to_spend_draft = cash_left_to_spend;
	for (size_t i = 0; i < scaled_needs.size(); i++) {
		auto [good_definition, max_quantity_to_buy] = *scaled_needs.nth(i);
		const fixed_point_t max_money_to_spend = market_instance.get_max_money_to_allocate_to_buy_quantity(
			*good_definition,
			max_quantity_to_buy
		);
		if (money_to_spend_per_good_draft[good_definition] >= max_money_to_spend) {
			continue;
		}

		fixed_point_t price_inverse = market_instance.get_price_inverse(*good_definition);
		fixed_point_t cash_available_for_good = fixed_point_t::mul_div(
			cash_left_to_spend_draft,
			price_inverse,
			price_inverse_sum
		);

		if (cash_available_for_good >= max_money_to_spend) {
			cash_left_to_spend_draft -= max_money_to_spend;
			money_to_spend_per_good_draft[good_definition] = max_money_to_spend;
			price_inverse_sum -= price_inverse;
			i = -1; //Restart loop and skip maxed out needs. This is required to spread the remaining cash again.
		} else {
			const fixed_point_t max_possible_quantity_bought = cash_available_for_good / market_instance.get_min_next_price(*good_definition);
			if (max_possible_quantity_bought < fixed_point_t::epsilon()) {
				money_to_spend_per_good_draft[good_definition] = fixed_point_t::_0();
			} else {
				money_to_spend_per_good_draft[good_definition] = cash_available_for_good;
			}
		}
	}

	for (auto const& [good, money_to_spend] : money_to_spend_per_good_draft) {
		money_to_spend_per_good[good] += money_to_spend;
		cash_left_to_spend -= money_to_spend;
	}

	reusable_map_0.clear();
}

void Pop::pop_tick(PopValuesFromProvince& shared_values) {
	pop_tick_without_cleanup(shared_values);
	for (auto& map : shared_values.reusable_maps) {
		map.clear();
	}
}

void Pop::pop_tick_without_cleanup(PopValuesFromProvince& shared_values) {
	DO_FOR_ALL_TYPES_OF_POP_INCOME(SET_TO_ZERO)
	DO_FOR_ALL_TYPES_OF_POP_EXPENSES(SET_TO_ZERO)
	#undef SET_TO_ZERO
	income = expenses = fixed_point_t::_0();
	employed = 0;

	auto& [
		reusable_map_0,
		reusable_map_1,
		reusable_map_2,
		reusable_map_3
	] = shared_values.reusable_maps;
	GoodDefinition::good_definition_map_t& max_quantity_to_buy_per_good = reusable_map_2;
	GoodDefinition::good_definition_map_t& money_to_spend_per_good = reusable_map_3;
	cash_allocated_for_artisanal_spending = fixed_point_t::_0();
	fill_needs_fulfilled_goods_with_false();
	if (artisanal_producer_nullable != nullptr) {
		//execute artisan_tick before needs
		artisanal_producer_nullable->artisan_tick(
			*this,
			max_quantity_to_buy_per_good,
			money_to_spend_per_good,
			reusable_map_0,
			reusable_map_1
		);
	}

	PopType const& type_never_null = *type;
	ProvinceInstance& location_never_null = *location;
	PopStrataValuesFromProvince const& shared_strata_values = shared_values.get_effects_per_strata()[type_never_null.get_strata()];
	PopsDefines const& defines = shared_values.get_defines();
	const fixed_point_t base_needs_scalar = (
		fixed_point_t::_1() + 2 * consciousness / defines.get_pdef_base_con()
	) * size;
	constexpr int32_t size_denominator = 200000;

	CountryInstance* country_to_report_economy_nullable = location_never_null.get_country_to_report_economy();
	#define FILL_NEEDS(need_category) \
		need_category##_needs.clear(); \
		const fixed_point_t need_category##_needs_scalar = base_needs_scalar * shared_strata_values.get_shared_##need_category##_needs_scalar(); \
		fixed_point_t need_category##_needs_price_inverse_sum = fixed_point_t::_0(); \
		if (OV_likely(need_category##_needs_scalar > fixed_point_t::_0())) { \
			need_category##_needs_acquired_quantity = need_category##_needs_desired_quantity = fixed_point_t::_0(); \
			for (auto [good_definition, quantity] : type_never_null.get_##need_category##_needs()) { \
				if (!market_instance.get_is_available(*good_definition)) { \
					continue; \
				} \
				fixed_point_t max_quantity_to_buy = quantity * need_category##_needs_scalar / size_denominator; \
				if (max_quantity_to_buy == fixed_point_t::_0()) { \
					continue; \
				} \
				if (country_to_report_economy_nullable != nullptr) { \
					country_to_report_economy_nullable->report_pop_need_demand(*type, *good_definition, max_quantity_to_buy); \
				} \
				need_category##_needs_desired_quantity += max_quantity_to_buy; \
				if (artisanal_produce_left_to_sell > fixed_point_t::_0() \
					&& &artisanal_producer_nullable->get_production_type().get_output_good() == good_definition \
				) { \
					const fixed_point_t own_produce_consumed = std::min(artisanal_produce_left_to_sell, max_quantity_to_buy); \
					artisanal_produce_left_to_sell -= own_produce_consumed; \
					max_quantity_to_buy -= own_produce_consumed; \
					need_category##_needs_acquired_quantity += own_produce_consumed; \
					if (country_to_report_economy_nullable != nullptr) { \
						country_to_report_economy_nullable->report_pop_need_consumption(type_never_null, *good_definition, own_produce_consumed); \
					} \
				} \
				if (OV_likely(max_quantity_to_buy > 0)) { \
					need_category##_needs_price_inverse_sum += market_instance.get_price_inverse(*good_definition); \
					need_category##_needs[good_definition] += max_quantity_to_buy; \
					max_quantity_to_buy_per_good[good_definition] += max_quantity_to_buy; \
				} \
			} \
		}

	DO_FOR_ALL_NEED_CATEGORIES(FILL_NEEDS)
	#undef FILL_NEEDS

	//It's safe to use cash as this happens before cash is updated via spending
	fixed_point_t cash_left_to_spend = cash.get_copy_of_value() - cash_allocated_for_artisanal_spending;

	#define ALLOCATE_FOR_NEEDS(need_category) \
		if (cash_left_to_spend > fixed_point_t::_0()) { \
			allocate_for_needs( \
				need_category##_needs, \
				money_to_spend_per_good, \
				reusable_map_0, \
				need_category##_needs_price_inverse_sum, \
				cash_left_to_spend \
			); \
		}

	DO_FOR_ALL_NEED_CATEGORIES(ALLOCATE_FOR_NEEDS)
	#undef ALLOCATE_FOR_NEEDS

	for (auto [good_definition, money_to_spend] : money_to_spend_per_good) {
		const fixed_point_t max_quantity_to_buy = max_quantity_to_buy_per_good[good_definition];

		if (OV_unlikely(max_quantity_to_buy <= fixed_point_t::_0())) {
			Logger::error("Aborted placing buy order for ", *good_definition, " of max_quantity_to_buy 0.");
			continue;
		}

		market_instance.place_buy_up_to_order(BuyUpToOrder {
			*good_definition,
			max_quantity_to_buy,
			money_to_spend,
			this,
			after_buy
		});
	}

	if (artisanal_produce_left_to_sell > fixed_point_t::_0()) {
		market_instance.place_market_sell_order(MarketSellOrder {
			artisanal_producer_nullable->get_production_type().get_output_good(),
			artisanal_produce_left_to_sell,
			this,
			after_sell
		});
		artisanal_produce_left_to_sell = fixed_point_t::_0();
	}
}

void Pop::after_buy(void* actor, BuyResult const& buy_result) {
	Pop& pop = *static_cast<Pop*>(actor);
	GoodDefinition const& good_definition = buy_result.get_good_definition();
	const fixed_point_t quantity_bought = buy_result.get_quantity_bought();
	const fixed_point_t money_spent = buy_result.get_money_spent();

	fixed_point_t quantity_left_to_consume = quantity_bought;
	if (pop.artisanal_producer_nullable != nullptr) {
		if (quantity_left_to_consume <= fixed_point_t::_0()) {
			return;
		}
		const fixed_point_t quantity_added_to_stockpile = pop.artisanal_producer_nullable->add_to_stockpile(
			good_definition,
			quantity_left_to_consume
		);

		if (quantity_added_to_stockpile > fixed_point_t::_0()) {
			quantity_left_to_consume -= quantity_added_to_stockpile;
			const fixed_point_t expense = fixed_point_t::mul_div(
				money_spent,
				quantity_added_to_stockpile,
				quantity_bought
			);
			pop.add_artisan_inputs_expense(expense);
		}
	}

	CountryInstance* get_country_to_report_economy_nullable = pop.location->get_country_to_report_economy();
	#define CONSUME_NEED(need_category) \
		if (quantity_left_to_consume <= fixed_point_t::_0()) { \
			return; \
		} \
		const GoodDefinition::good_definition_map_t::const_iterator need_category##it = pop.need_category##_needs.find(&good_definition); \
		if (need_category##it != pop.need_category##_needs.end()) { \
			const fixed_point_t desired_quantity = need_category##it->second; \
			fixed_point_t consumed_quantity; \
			if (quantity_left_to_consume >= desired_quantity) { \
				consumed_quantity = desired_quantity; \
				pop.need_category##_needs_fulfilled_goods.at(&good_definition) = true; \
			} else { \
				consumed_quantity = quantity_left_to_consume; \
			} \
			pop.need_category##_needs_acquired_quantity += consumed_quantity; \
			quantity_left_to_consume -= consumed_quantity; \
			if (get_country_to_report_economy_nullable != nullptr) { \
				get_country_to_report_economy_nullable->report_pop_need_consumption(*pop.type, good_definition, consumed_quantity); \
			} \
			const fixed_point_t expense = fixed_point_t::mul_div( \
				money_spent, \
				consumed_quantity, \
				quantity_bought \
			); \
			pop.add_##need_category##_needs_expense(expense); \
		}

	DO_FOR_ALL_NEED_CATEGORIES(CONSUME_NEED)
	#undef CONSUME_NEED
}

void Pop::after_sell(void* actor, SellResult const& sell_result) {
	if (sell_result.get_money_gained() > fixed_point_t::_0()) {
		static_cast<Pop*>(actor)->add_artisanal_income(sell_result.get_money_gained());
	}
}

void Pop::allocate_cash_for_artisanal_spending(fixed_point_t money_to_spend) {
	cash_allocated_for_artisanal_spending += money_to_spend;
}

void Pop::report_artisanal_produce(const fixed_point_t quantity) {
	artisanal_produce_left_to_sell = quantity;
}

void Pop::hire(pop_size_t count) {
	if (OV_unlikely(count <= 0)) {
		Logger::warning("Tried employing non-positive number of pops. ", count, " Context", get_pop_context_text().str());
	}
	employed += count;
	if (OV_unlikely(employed > size)) {
		Logger::error("Employed count became greater than pop size. ", employed, " Context", get_pop_context_text().str());
	} else if (OV_unlikely(employed < 0)) {
		Logger::error("Employed count became negative. ", employed ," Context", get_pop_context_text().str());
	}
}

#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
#undef DO_FOR_ALL_TYPES_OF_POP_EXPENSES
#undef DO_FOR_ALL_NEED_CATEGORIES