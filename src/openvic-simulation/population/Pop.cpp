#include "Pop.hpp"
#include "PopDeps.hpp"

#include <concepts> // IWYU pragma: keep for lambda
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/core/FormatValidate.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/IssueManager.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/population/PopNeedsMacro.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/population/Religion.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/scripts/Condition.hpp"


using namespace OpenVic;

PopBase::PopBase(
	PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
	fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
) : type { &new_type }, culture { new_culture }, religion { new_religion }, size { new_size }, militancy { new_militancy },
	consciousness { new_consciousness }, rebel_type { new_rebel_type } {}

Pop::Pop(
	PopBase const& pop_base,
	decltype(supporter_equivalents_by_ideology)::keys_span_type ideology_keys,
	PopDeps const& pop_deps,
	const pop_id_in_province_t new_id_in_province
)
  : PopBase { pop_base },
  	id_in_province { new_id_in_province },
	market_instance { pop_deps.market_instance },
	artisanal_producer_optional {
		type->is_artisan
			? std::optional<ArtisanalProducer> {
				pop_deps.artisanal_producer_deps
			}
			: std::optional<ArtisanalProducer> {}
	},
	supporter_equivalents_by_ideology { ideology_keys } {
		reserve_needs_fulfilled_goods();
	}

fixed_point_t Pop::get_unemployment_fraction() const {
	if (!type->can_be_unemployed) {
		return 0;
	}
	return fixed_point_t::from_fraction(get_unemployed(), size);
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
	auto test_weight_indexed = []<typename U>(IndexedFlatMap<U, fixed_point_t>& map, U const& key, int32_t min, int32_t max) -> void {
		const int32_t value = rand() % (max + 1);
		if (value >= min) {
			map.set(key, value);
		}
	};
	auto test_weight_ordered = []<typename T, typename U>(ordered_map<T const*, fixed_point_t>& map, U const& key, int32_t min, int32_t max) -> void {
		if constexpr (std::is_convertible_v<U const*, T const*> || std::is_convertible_v<U, T const*>) {
			const int32_t value = rand() % (max + 1);
			if (value >= min) {
				if constexpr (std::is_pointer_v<U>) {
					map.insert_or_assign(key, value);
				} else {
					map.insert_or_assign(&key, value);
				}
			}
		} else {
			// Optional: Handle the case where the conversion is not possible, perhaps with a static_assert.
			static_assert(std::is_convertible_v<U const*, T const*> || std::is_convertible_v<U, T const*>, "Type U is not convertible to T const*.");
		}
	};

	/* All entries equally weighted for testing. */
	supporter_equivalents_by_ideology.fill(0);
	for (Ideology const& ideology : supporter_equivalents_by_ideology.get_keys()) {
		test_weight_indexed(supporter_equivalents_by_ideology, ideology, 1, 5);
	}
	supporter_equivalents_by_ideology.rescale(type_safe::get(size));

	supporter_equivalents_by_issue.clear();
	for (BaseIssue const& issue : issue_manager.get_party_policies()) {
		test_weight_ordered(supporter_equivalents_by_issue, issue, 3, 6);
	}
	for (Reform const& reform : issue_manager.get_reforms()) {
		if (!reform.get_reform_group().is_uncivilised()) {
			test_weight_ordered(supporter_equivalents_by_issue, reform, 3, 6);
		}
	}
	rescale_fixed_point_map(supporter_equivalents_by_issue, type_safe::get(size));

	if (!vote_equivalents_by_party.empty()) {
		for (auto& [party, value] : vote_equivalents_by_party) {
			vote_equivalents_by_party[party] = 0;
			test_weight_ordered(vote_equivalents_by_party, party, 4, 10);
		}
		rescale_fixed_point_map(vote_equivalents_by_party, type_safe::get(size));
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
	life_needs_desired_quantity = everyday_needs_desired_quantity = luxury_needs_desired_quantity = 1;
}

bool Pop::convert_to_equivalent() {
	PopType const* const equivalent = get_type()->get_equivalent();
	if (equivalent == nullptr) {
		spdlog::error_s("Tried to convert pop of type {} to equivalent, but there is no equivalent.", *get_type());
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
	vote_equivalents_by_party.clear();
	if (location == nullptr) {
		return;
	}
	CountryInstance const* owner = location->get_owner();
	if (owner == nullptr) {
		return;
	}
	CountryDefinition const& country_definition = owner->country_definition;

	auto view = country_definition.get_parties() | std::views::transform(
		[](CountryParty const& key) {
			return std::make_pair(&key, fixed_point_t::_0);
		}
	);
	vote_equivalents_by_party.insert(view.begin(), view.end());
	// TODO - calculate vote distribution
}

fixed_point_t Pop::get_supporter_equivalents_by_ideology(Ideology const& ideology) const {
	return supporter_equivalents_by_ideology.at(ideology);
}

fixed_point_t Pop::get_supporter_equivalents_by_issue(BaseIssue const& issue) const {
	const decltype(supporter_equivalents_by_issue)::const_iterator it = supporter_equivalents_by_issue.find(&issue);

	if (it != supporter_equivalents_by_issue.end()) {
		return it->second;
	} else {
		return 0;
	}
}

fixed_point_t Pop::get_vote_equivalents_by_party(CountryParty const& party) const {
	const decltype(vote_equivalents_by_party)::const_iterator it = vote_equivalents_by_party.find(&party);
	if (it == vote_equivalents_by_party.end()) {
		return 0;
	}
	return it.value();
}

void Pop::update_gamestate(
	MilitaryDefines const& military_defines,
	CountryInstance const* owner,
	const fixed_point_t pop_size_per_regiment_multiplier
) {
	using enum culture_status_t;

	if (owner != nullptr) {
		if (owner->is_primary_culture(culture)) {
			culture_status = PRIMARY;
		} else if (owner->is_accepted_culture(culture)) {
			culture_status = ACCEPTED;
		} else {
			culture_status = UNACCEPTED;
		}
	} else {
		culture_status = UNACCEPTED;
	}

	static constexpr fixed_point_t MIN_MILITANCY = 0;
	static constexpr fixed_point_t MAX_MILITANCY = 10;
	static constexpr fixed_point_t MIN_CONSCIOUSNESS = 0;
	static constexpr fixed_point_t MAX_CONSCIOUSNESS = 10;
	static constexpr fixed_point_t MIN_LITERACY = fixed_point_t::_0_01;
	static constexpr fixed_point_t MAX_LITERACY = 1;

	militancy = std::clamp(militancy, MIN_MILITANCY, MAX_MILITANCY);
	consciousness = std::clamp(consciousness, MIN_CONSCIOUSNESS, MAX_CONSCIOUSNESS);
	literacy = std::clamp(literacy, MIN_LITERACY, MAX_LITERACY);

	if (
		size < military_defines.get_min_pop_size_for_regiment() || owner == nullptr ||
		!is_culture_status_allowed(owner->get_allowed_regiment_cultures(), culture_status)
	) {
		max_supported_regiments = 0;
	} else {
		max_supported_regiments = (
			type_safe::get(size) / (type_safe::get(military_defines.get_pop_size_per_regiment()) * pop_size_per_regiment_multiplier) //
		).floor<size_t>() + 1;
	}
}

memory::string Pop::get_pop_context_text() const {
	return memory::fmt::format(
		"location: {} type: {} culture: {} religion: {} size: {}",
		ovfmt::validate(location), *type, culture, religion, size
	);
}

void Pop::reserve_needs_fulfilled_goods() {
	PopType const& type_never_null = *type;
	#define RESERVE_NEEDS(need_category) \
		need_category##_needs_fulfilled_goods.reserve(type_never_null.get_##need_category##_needs().size());

	OV_DO_FOR_ALL_NEED_CATEGORIES(RESERVE_NEEDS)
	#undef RESERVE_NEEDS
}

void Pop::fill_needs_fulfilled_goods_with_false() {
	PopType const& type_never_null = *type;
	#define FILL_WITH_FALSE(need_category) \
		need_category##_needs_fulfilled_goods.clear(); \
		for (auto [good, base_demand] : type_never_null.get_##need_category##_needs()) { \
			need_category##_needs_fulfilled_goods.emplace(good, false); \
		}

	OV_DO_FOR_ALL_NEED_CATEGORIES(FILL_WITH_FALSE)
	#undef FILL_WITH_FALSE
}

void Pop::pay_income_tax(fixed_point_t& income) {
	CountryInstance* const tax_collector_nullable = location->get_country_to_report_economy();
	if (tax_collector_nullable == nullptr) {
		return;
	}
	const fixed_point_t effective_tax_rate = tax_collector_nullable->get_effective_tax_rate_by_strata(type->strata).get_untracked();
	const fixed_point_t tax = effective_tax_rate * income;
	tax_collector_nullable->report_pop_income_tax(*type, income, tax);
	income -= tax;
}

template<bool IsTaxable>
void Pop::add_artisanal_revenue(const fixed_point_t revenue) {
	if (OV_unlikely(revenue == 0)) {
		spdlog::warn_s("Adding artisanal_revenue of 0 to pop. Context{}", get_pop_context_text());
		return;
	}

	if (OV_unlikely(revenue < 0)) {
		spdlog::error_s("Adding negative artisanal_revenue of {} to pop. Context{}", revenue, get_pop_context_text());
		return;
	}

	fixed_point_t income;
	if constexpr (IsTaxable) {
		if (OV_unlikely(!artisanal_producer_optional.has_value())) {
			income = revenue;
		} else {
			income = std::max(
				fixed_point_t::_0,
				revenue - artisanal_producer_optional->get_costs_of_production()
			);
		}

		pay_income_tax(income);
	} else {
		income = revenue;
	}

	artisanal_revenue += revenue;
	income += income;
	cash += income;
}
template void Pop::add_artisanal_revenue<true>(const fixed_point_t revenue);
template void Pop::add_artisanal_revenue<false>(const fixed_point_t revenue);

#define DEFINE_ADD_INCOME_FUNCTIONS(name) \
	void Pop::add_##name(fixed_point_t amount){ \
		if (OV_unlikely(amount == 0)) { \
			spdlog::warn_s("Adding " #name " of 0 to pop. Context{}", get_pop_context_text()); \
			return; \
		} \
		if (OV_unlikely(amount < 0)) { \
			spdlog::error_s("Adding negative " #name " of {} to pop. Context{}", amount, get_pop_context_text()); \
			return; \
		} \
		pay_income_tax(amount); \
		name += amount; \
		income += amount; \
		cash += amount; \
	}

OV_DO_FOR_ALL_TYPES_OF_POP_INCOME(DEFINE_ADD_INCOME_FUNCTIONS)
#undef DEFINE_ADD_INCOME_FUNCTIONS

#define DEFINE_ADD_EXPENSE_FUNCTIONS(name) \
	void Pop::add_##name(const fixed_point_t amount){ \
		if (OV_unlikely(amount == 0)) { \
			spdlog::warn_s("Adding " #name " of 0 to pop. Context:{}", get_pop_context_text()); \
			return; \
		} \
		name += amount; \
		const fixed_point_t expenses_copy = expenses += amount; \
		if (OV_unlikely(expenses_copy < 0)) { \
			spdlog::error_s( \
				"Total expenses became negative ({}) after adding " #name " of {} to pop. Context:{}", \
				expenses_copy, amount, get_pop_context_text() \
			); \
		} \
		const fixed_point_t cash_copy = cash -= amount; \
		if (OV_unlikely(cash_copy < 0)) { \
			spdlog::error_s( \
				"Total cash became negative ({}) after adding " #name " of {} to pop. Context:{}", \
				cash_copy, amount, get_pop_context_text() \
			); \
		} \
	}

OV_DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DEFINE_ADD_EXPENSE_FUNCTIONS)
#undef DEFINE_ADD_EXPENSE_FUNCTIONS

void Pop::add_import_subsidies(const fixed_point_t amount) {
	//It's not income, otherwise we'd pay income tax.
	//It's not a negative expense, otherwise total expense might go negative.
	cash += amount;
}

#define DEFINE_NEEDS_FULFILLED(need_category) \
	fixed_point_t Pop::get_##need_category##_needs_fulfilled() const { \
		const fixed_point_t desired_quantity_copy = need_category##_needs_desired_quantity.get_copy_of_value(); \
		if (desired_quantity_copy == 0) { \
			return 1; \
		} \
		return need_category##_needs_acquired_quantity.get_copy_of_value() / desired_quantity_copy; \
	}
OV_DO_FOR_ALL_NEED_CATEGORIES(DEFINE_NEEDS_FULFILLED)
#undef DEFINE_NEEDS_FULFILLED

void Pop::allocate_for_needs(
	fixed_point_map_t<GoodDefinition const*> const& scaled_needs,
	forwardable_span<fixed_point_t> money_to_spend_per_good,
	memory::vector<fixed_point_t>& reusable_vector,
	fixed_point_t& weights_sum,
	fixed_point_t& cash_left_to_spend
) {
	if (weights_sum <= 0) {
		return;
	}

	memory::vector<fixed_point_t>& money_to_spend_per_good_draft = reusable_vector;
	money_to_spend_per_good_draft.resize(scaled_needs.size(), 0);
	fixed_point_t cash_left_to_spend_draft = cash_left_to_spend;

	bool needs_redistribution = true;
	while (needs_redistribution) {
		needs_redistribution = false;
		for (auto it = scaled_needs.begin(); it < scaled_needs.end(); it++) {
			GoodDefinition const* const good_definition_ptr = it.key();
			GoodDefinition const& good_definition = *good_definition_ptr;
			const fixed_point_t max_quantity_to_buy = it.value();
			const ptrdiff_t i = it - scaled_needs.begin();
			const fixed_point_t max_money_to_spend = market_instance.get_max_money_to_allocate_to_buy_quantity(
				good_definition,
				max_quantity_to_buy
			);
			if (money_to_spend_per_good_draft[i] >= max_money_to_spend) {
				continue;
			}

			fixed_point_t weight = market_instance.get_good_instance(good_definition).get_price_inverse();
			fixed_point_t cash_available_for_good = fixed_point_t::mul_div(
				cash_left_to_spend_draft,
				weight,
				weights_sum
			);

			if (cash_available_for_good >= max_money_to_spend) {
				cash_left_to_spend_draft -= max_money_to_spend;
				money_to_spend_per_good_draft[i] = max_money_to_spend;
				weights_sum -= weight;
				needs_redistribution = weights_sum > 0;
				break;
			}

			const fixed_point_t max_possible_quantity_bought = cash_available_for_good / market_instance.get_min_next_price(good_definition);
			if (max_possible_quantity_bought < fixed_point_t::epsilon) {
				money_to_spend_per_good_draft[i] = 0;
			} else {
				money_to_spend_per_good_draft[i] = cash_available_for_good;
			}
		}
	}

	for (auto it = scaled_needs.begin(); it < scaled_needs.end(); it++) {
		GoodDefinition const* const good_definition_ptr = it.key();
		GoodDefinition const& good_definition = *good_definition_ptr;
		const ptrdiff_t i = it - scaled_needs.begin();
		const fixed_point_t money_to_spend = money_to_spend_per_good_draft[i];
		money_to_spend_per_good[type_safe::get(good_definition.index)] += money_to_spend;
		cash_left_to_spend -= money_to_spend;
	}

	reusable_vector.clear();
}

bool Pop::evaluate_leaf(ConditionNode const& node) const {
	std::string_view const& id = node.get_condition()->get_identifier();

	if (id == "consciousness") {
		fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
		return get_consciousness() >= expected;
	}
	// TODO: Implement the rest
	spdlog::warn_s("Condition {} not implemented in Pop::evaluate_leaf", id);
	return false;
}

void Pop::pop_tick(
	PopValuesFromProvince const& shared_values,
	RandomU32& random_number_generator,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_POP_TICK
	> reusable_vectors
) {
	pop_tick_without_cleanup(
		shared_values,
		random_number_generator,
		reusable_goods_mask,
		reusable_vectors
	);
	for (auto& reusable_vector : reusable_vectors) {
		reusable_vector.clear();
	}
}

void Pop::pop_tick_without_cleanup(
	PopValuesFromProvince const& shared_values,
	RandomU32& random_number_generator,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_POP_TICK
	> reusable_vectors
) {
	forwardable_span<const GoodDefinition> good_keys = reusable_goods_mask.get_keys();
	memory::vector<fixed_point_t>& reusable_vector_0 = reusable_vectors[0];
	memory::vector<fixed_point_t>& reusable_vector_1 = reusable_vectors[1];
	memory::vector<fixed_point_t>& max_quantity_to_buy_per_good = reusable_vectors[2];
	max_quantity_to_buy_per_good.resize(good_keys.size(), 0);
	memory::vector<fixed_point_t>& money_to_spend_per_good = reusable_vectors[3];
	money_to_spend_per_good.resize(good_keys.size(), 0);
	cash_allocated_for_artisanal_spending = 0;
	fill_needs_fulfilled_goods_with_false();
	
	fixed_point_map_t<GoodDefinition const*> goods_to_sell {};
	if (artisanal_producer_optional.has_value()) {
		//execute artisan_tick before needs
		ArtisanalProducer& artisanal_producer = artisanal_producer_optional.value();
		artisanal_producer.artisan_tick(
			market_instance,
			*this,
			shared_values,
			random_number_generator,
			reusable_goods_mask,
			max_quantity_to_buy_per_good,
			money_to_spend_per_good,
			reusable_vector_0,
			reusable_vector_1,
			goods_to_sell
		);
	}

	//after artisan_tick as it uses income & expenses
	#define SET_TO_ZERO(name) \
		name = 0;

	OV_DO_FOR_ALL_TYPES_OF_POP_INCOME(SET_TO_ZERO)
	OV_DO_FOR_ALL_TYPES_OF_POP_EXPENSES(SET_TO_ZERO)
	#undef SET_TO_ZERO
	income = expenses = 0;

	ProvinceInstance& location_never_null = *location;
	CountryInstance* const country_to_report_economy_nullable = location_never_null.get_country_to_report_economy();

	if (country_to_report_economy_nullable != nullptr) {
		country_to_report_economy_nullable->request_salaries_and_welfare_and_import_subsidies(*this);
	}
	
	//unemployment subsidies are based on yesterdays unemployment
	employed = 0;
	//import subsidies are based on yesterday
	yesterdays_import_value = 0;

	PopType const& type_never_null = *type;
	PopStrataValuesFromProvince const& shared_strata_values = shared_values.get_effects_by_strata(type_never_null.strata);
	PopsDefines const& defines = shared_values.defines;
	const fixed_point_t base_needs_scalar = (
		fixed_point_t::_1 + 2 * consciousness / defines.get_pdef_base_con()
	) * size;

	#define FILL_NEEDS(need_category) \
		need_category##_needs.clear(); \
		const fixed_point_t need_category##_needs_scalar = base_needs_scalar * shared_strata_values.get_shared_##need_category##_needs_scalar(); \
		fixed_point_t need_category##_needs_price_inverse_sum = 0; \
		if (OV_likely(need_category##_needs_scalar > 0)) { \
			need_category##_needs_acquired_quantity = need_category##_needs_desired_quantity = 0; \
			for (auto [good_definition_ptr, quantity] : type_never_null.get_##need_category##_needs()) { \
				GoodDefinition const& good_definition = *good_definition_ptr; \
				if (!market_instance.get_is_available(good_definition)) { \
					continue; \
				} \
				fixed_point_t max_quantity_to_buy = quantity * need_category##_needs_scalar / size_denominator; \
				if (max_quantity_to_buy == 0) { \
					continue; \
				} \
				if (country_to_report_economy_nullable != nullptr) { \
					country_to_report_economy_nullable->report_pop_need_demand(*type, good_definition, max_quantity_to_buy); \
				} \
				need_category##_needs_desired_quantity += max_quantity_to_buy; \
				auto goods_to_sell_iterator = goods_to_sell.find(good_definition_ptr); \
				if (goods_to_sell_iterator != goods_to_sell.end() && goods_to_sell_iterator.value() > 0) { \
					const fixed_point_t own_produce_consumed = std::min(goods_to_sell_iterator.value(), max_quantity_to_buy); \
					goods_to_sell_iterator.value() -= own_produce_consumed; \
					max_quantity_to_buy -= own_produce_consumed; \
					need_category##_needs_acquired_quantity += own_produce_consumed; \
					if (country_to_report_economy_nullable != nullptr) { \
						country_to_report_economy_nullable->report_pop_need_consumption(type_never_null, good_definition, own_produce_consumed); \
					} \
				} \
				if (OV_likely(max_quantity_to_buy > 0)) { \
					need_category##_needs_price_inverse_sum += market_instance.get_good_instance(good_definition).get_price_inverse(); \
					need_category##_needs[good_definition_ptr] += max_quantity_to_buy; \
					max_quantity_to_buy_per_good[type_safe::get(good_definition.index)] += max_quantity_to_buy; \
				} \
			} \
		}

	OV_DO_FOR_ALL_NEED_CATEGORIES(FILL_NEEDS)
	#undef FILL_NEEDS

	//It's safe to use cash as this happens before cash is updated via spending
	fixed_point_t cash_left_to_spend = cash.get_copy_of_value() / shared_values.get_max_cost_multiplier()
		- cash_allocated_for_artisanal_spending;

	#define ALLOCATE_FOR_NEEDS(need_category) \
		if (cash_left_to_spend > 0) { \
			allocate_for_needs( \
				need_category##_needs, \
				money_to_spend_per_good, \
				reusable_vector_0, \
				need_category##_needs_price_inverse_sum, \
				cash_left_to_spend \
			); \
		}

	OV_DO_FOR_ALL_NEED_CATEGORIES(ALLOCATE_FOR_NEEDS)
	#undef ALLOCATE_FOR_NEEDS

	const std::optional<country_index_t> country_index_optional = country_to_report_economy_nullable == nullptr
		? std::nullopt
		: std::optional<country_index_t>{country_to_report_economy_nullable->index};

	for (auto it = good_keys.begin(); it < good_keys.end(); it++) {
		const ptrdiff_t i = it - good_keys.begin();
		const fixed_point_t max_quantity_to_buy = max_quantity_to_buy_per_good[i];

		if (max_quantity_to_buy <= 0) {
			continue;
		}
		
		GoodDefinition const& good_definition = *it;
		const fixed_point_t money_to_spend = money_to_spend_per_good[i];

		market_instance.place_buy_up_to_order({
			good_definition,
			country_index_optional,
			max_quantity_to_buy,
			money_to_spend,
			this,
			after_buy
		});
	}

	for (const auto [good_ptr, quantity_to_sell] : goods_to_sell) {
		GoodDefinition const& good = *good_ptr;
		if (quantity_to_sell <= 0) {
			if (OV_unlikely(quantity_to_sell < 0)) {
				spdlog::error_s("Pop had negative quantity {} left to sell of good {}.", quantity_to_sell, good);
			}
			continue;
		}

		market_instance.place_market_sell_order(
			{
				good,
				country_index_optional,
				quantity_to_sell,
				this,
				after_sell
			},
			reusable_vectors[4]
		);
	}
}

void Pop::after_buy(void* actor, BuyResult const& buy_result) {
	const fixed_point_t quantity_bought = buy_result.quantity_bought;

	if (quantity_bought == 0) {
		return;
	}

	Pop& pop = *static_cast<Pop*>(actor);	
	ProvinceInstance& location_never_null = *pop.get_location();
	CountryInstance* const country_to_report_economy_nullable = location_never_null.get_country_to_report_economy();

	fixed_point_t money_spent = buy_result.money_spent_total;
	pop.yesterdays_import_value += buy_result.money_spent_on_imports;
	if (country_to_report_economy_nullable != nullptr) {
		const fixed_point_t tariff = country_to_report_economy_nullable->apply_tariff(buy_result.money_spent_on_imports);
		money_spent += tariff;
	}

	GoodDefinition const& good_definition = buy_result.good_definition;
	fixed_point_t quantity_left_to_consume = quantity_bought;
	if (pop.artisanal_producer_optional.has_value()) {
		if (quantity_left_to_consume <= 0) {
			return;
		}
		const fixed_point_t quantity_added_to_stockpile = pop.artisanal_producer_optional.value().add_to_stockpile(
			good_definition,
			quantity_left_to_consume
		);

		if (quantity_added_to_stockpile > 0) {
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
		if (quantity_left_to_consume <= 0) { \
			return; \
		} \
		const fixed_point_map_t<GoodDefinition const*>::const_iterator need_category##it = pop.need_category##_needs.find(&good_definition); \
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

	OV_DO_FOR_ALL_NEED_CATEGORIES(CONSUME_NEED)
	#undef CONSUME_NEED
}

void Pop::after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) {
	Pop& pop = *static_cast<Pop*>(actor);
	if (sell_result.money_gained > 0) {
		OV_ERR_FAIL_COND_MSG(!pop.artisanal_producer_optional.has_value(), "Pop is selling artisanal goods but has no artisan.");
		ArtisanalProducer& artisan = pop.artisanal_producer_optional.value();
		if (artisan.get_last_produced_good() != nullptr && *artisan.get_last_produced_good() == sell_result.good_definition) {
			pop.add_artisanal_revenue<true>(sell_result.money_gained);
		} else {
			pop.add_artisanal_revenue<false>(sell_result.money_gained);
		}
		artisan.subtract_from_stockpile(sell_result.good_definition, sell_result.quantity_sold);
	}
}

void Pop::allocate_cash_for_artisanal_spending(fixed_point_t money_to_spend) {
	cash_allocated_for_artisanal_spending += money_to_spend;
}

void Pop::hire(pop_size_t count) {
	if (OV_unlikely(count <= 0)) {
		spdlog::warn_s(
			"Tried employing non-positive number of pops. {} Context{}",
			count, get_pop_context_text()
		);
	}
	employed += count;
	if (OV_unlikely(employed > size)) {
		spdlog::error_s(
			"Employed count became greater than pop size. {} Context{}",
			employed, get_pop_context_text()
		);
	} else if (OV_unlikely(employed < 0)) {
		spdlog::error_s(
			"Employed count became negative. {} Context{}",
			employed, get_pop_context_text()
		);
	}
}

bool Pop::try_recruit() {
	if (regiment_count >= max_supported_regiments) {
		return false;
	}

	++regiment_count;
	return true;
}

bool Pop::try_recruit_understrength() {
	if (regiment_count > max_supported_regiments) {
		return false;
	}

	++regiment_count;
	return true;
}