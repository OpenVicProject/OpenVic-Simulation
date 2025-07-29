#pragma once

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerFactoryPattern.hpp"
#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/fixed_point/Atomic.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct DefineManager;
	struct MarketInstance;
	struct ProvinceInstance;
	struct PopValuesFromProvince;
	struct BuyResult;
	struct SellResult;

	struct PopBase {
		friend PopManager;

	protected:
		PopType const* PROPERTY_ACCESS(type, protected);
		Culture const& PROPERTY_ACCESS(culture, protected);
		Religion const& PROPERTY_ACCESS(religion, protected);
		pop_size_t PROPERTY_ACCESS(size, protected);
		fixed_point_t PROPERTY_RW_ACCESS(militancy, protected);
		fixed_point_t PROPERTY_RW_ACCESS(consciousness, protected);
		RebelType const* PROPERTY_ACCESS(rebel_type, protected);

		PopBase(
			PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
			fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
		);
	};

	#define DO_FOR_ALL_TYPES_OF_POP_INCOME(F) \
		F(rgo_owner_income) \
		F(rgo_worker_income) \
		F(artisanal_income) \
		F(factory_worker_income) \
		F(factory_owner_income) \
		F(unemployment_subsidies) \
		F(pensions) \
		F(government_salary_administration) \
		F(government_salary_education) \
		F(government_salary_military) \
		F(event_and_decision_income) \
		F(loan_interest_payments)

	#define DO_FOR_ALL_TYPES_OF_POP_EXPENSES(F) \
		F(life_needs_expense) \
		F(everyday_needs_expense) \
		F(luxury_needs_expense) \
		F(artisan_inputs_expense)

	#define DECLARE_POP_MONEY_STORES(money_type) \
		fixed_point_t PROPERTY(money_type);

	#define DECLARE_POP_MONEY_STORE_FUNCTIONS(name) \
		void add_##name(const fixed_point_t amount);

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop : PopBase {
		friend struct ProvinceInstance;

		enum struct culture_status_t : uint8_t {
			UNACCEPTED, ACCEPTED, PRIMARY
		};

		static constexpr bool is_culture_status_allowed(RegimentType::allowed_cultures_t allowed, culture_status_t status) {
			return static_cast<uint8_t>(allowed) <= static_cast<uint8_t>(status);
		}

		static constexpr pop_size_t MAX_SIZE = std::numeric_limits<pop_size_t>::max();

	private:
		memory::unique_ptr<ArtisanalProducer> artisanal_producer_nullable;
		fixed_point_t cash_allocated_for_artisanal_spending;
		fixed_point_t artisanal_produce_left_to_sell;
		ProvinceInstance* PROPERTY_PTR(location, nullptr);
		MarketInstance& PROPERTY(market_instance);

		/* Last day's size change by source. */
		pop_size_t PROPERTY(total_change, 0);
		pop_size_t PROPERTY(num_grown, 0);
		pop_size_t PROPERTY(num_promoted, 0); // TODO - detailed promotion/demotion info (what to)
		pop_size_t PROPERTY(num_demoted, 0);
		pop_size_t PROPERTY(num_migrated_internal, 0); // TODO - detailed migration info (where to)
		pop_size_t PROPERTY(num_migrated_external, 0);
		pop_size_t PROPERTY(num_migrated_colonial, 0);

		culture_status_t PROPERTY(culture_status, culture_status_t::UNACCEPTED);

		static constexpr fixed_point_t DEFAULT_POP_LITERACY = fixed_point_t::_0_10;
		fixed_point_t PROPERTY_RW(literacy, DEFAULT_POP_LITERACY);

		// All of these should have a total size equal to the pop size, allowing the distributions from different pops to be
		// added together with automatic weighting based on their relative sizes. Similarly, the province, state and country
		// equivalents of these distributions will have a total size equal to their total population size.
		IndexedFlatMap<Ideology, fixed_point_t> PROPERTY(ideology_distribution);
		fixed_point_map_t<Issue const*> PROPERTY(issue_distribution);
		IndexedMap<CountryParty, fixed_point_t> PROPERTY(vote_distribution);

		pop_size_t employed = 0;
	public:
		static constexpr pop_size_t size_denominator = 200000;
		static constexpr size_t VECTORS_FOR_POP_TICK = 5;

		constexpr pop_size_t get_unemployed() const {
			return size - employed;
		}
		constexpr fixed_point_t get_unemployment_fraction() const {
			if (!type->get_can_be_unemployed()) {
				return 0;
			}
			return fixed_point_t::parse(get_unemployed()) / size;
		}
	private:
		fixed_point_t PROPERTY(income);
		fixed_point_t PROPERTY(savings);
		moveable_atomic_fixed_point_t PROPERTY(cash);
		moveable_atomic_fixed_point_t PROPERTY(expenses); //positive value means POP paid for goods. This is displayed * -1 in UI.
		moveable_atomic_fixed_point_t PROPERTY(yesterdays_import_value);

		#define NEED_MEMBERS(need_category) \
			moveable_atomic_fixed_point_t need_category##_needs_acquired_quantity, need_category##_needs_desired_quantity; \
			public: \
				fixed_point_t get_##need_category##_needs_fulfilled() const; \
			private: \
				GoodDefinition::good_definition_map_t PROPERTY(need_category##_needs); /* TODO pool? (if recalculating in UI is acceptable) */ \
				ordered_map<GoodDefinition const*, bool> PROPERTY(need_category##_needs_fulfilled_goods);

		DO_FOR_ALL_NEED_CATEGORIES(NEED_MEMBERS)
		#undef NEED_MEMBERS

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_MONEY_STORES);
		DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DECLARE_POP_MONEY_STORES);
		#undef DECLARE_POP_MONEY_STORES

		size_t PROPERTY(max_supported_regiments, 0);

		Pop(
			PopBase const& pop_base,
			decltype(ideology_distribution)::keys_span_type ideology_keys,
			MarketInstance& new_market_instance,
			ArtisanalProducerFactoryPattern& artisanal_producer_factory_pattern
		);

		memory::stringstream get_pop_context_text() const;
		void reserve_needs_fulfilled_goods();
		void fill_needs_fulfilled_goods_with_false();
		void allocate_for_needs(
			GoodDefinition::good_definition_map_t const& scaled_needs,
			utility::forwardable_span<fixed_point_t> money_to_spend_per_good,
			memory::vector<fixed_point_t>& reusable_vector,
			fixed_point_t& price_inverse_sum,
			fixed_point_t& cash_left_to_spend
		);
		void pop_tick_without_cleanup(
			PopValuesFromProvince& shared_values,
			utility::forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_POP_TICK
			> reusable_vectors
		);
		void pay_income_tax(fixed_point_t& income);
		static void after_buy(void* actor, BuyResult const& buy_result);
		//matching GoodMarketSellOrder::callback_t
		static void after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector);

	public:
		Pop(Pop const&) = delete;
		Pop(Pop&&) = default;
		Pop& operator=(Pop const&) = delete;
		Pop& operator=(Pop&&) = delete;

		void setup_pop_test_values(IssueManager const& issue_manager);
		bool convert_to_equivalent();

		void set_location(ProvinceInstance& new_location);
		void update_location_based_attributes();

		// The values returned by these functions are scaled by pop size, so they must be divided by pop size to get
		// the support as a proportion of 1.0
		fixed_point_t get_ideology_support(Ideology const& ideology) const;
		fixed_point_t get_issue_support(Issue const& issue) const;
		fixed_point_t get_party_support(CountryParty const& party) const;

		void update_gamestate(
			DefineManager const& define_manager, CountryInstance const* owner,
			const fixed_point_t pop_size_per_regiment_multiplier
		);

		DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_MONEY_STORE_FUNCTIONS)
		DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DECLARE_POP_MONEY_STORE_FUNCTIONS)
		DECLARE_POP_MONEY_STORE_FUNCTIONS(import_subsidies)
		#undef DECLARE_POP_MONEY_STORE_FUNCTIONS

		void pop_tick(
			PopValuesFromProvince& shared_values,
			utility::forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_POP_TICK
			> reusable_vectors
		);
		void allocate_cash_for_artisanal_spending(const fixed_point_t money_to_spend);
		void report_artisanal_produce(const fixed_point_t quantity);
		void hire(pop_size_t count);
	};
}
#ifndef KEEP_DO_FOR_ALL_TYPES_OF_INCOME
	#undef DO_FOR_ALL_TYPES_OF_POP_INCOME
#endif

#ifndef KEEP_DO_FOR_ALL_TYPES_OF_EXPENSES
	#undef DO_FOR_ALL_TYPES_OF_POP_EXPENSES
#endif

#undef DO_FOR_ALL_NEED_CATEGORIES
