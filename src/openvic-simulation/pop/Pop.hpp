#pragma once

#include "openvic-simulation/Alias.hpp"
#include "openvic-simulation/core/container/IndexedFlatMap.hpp"
#include "openvic-simulation/core/memory/FixedPointMap.hpp"
#include "openvic-simulation/core/memory/OrderedMap.hpp"
#include "openvic-simulation/core/object/FixedPoint.hpp"
#include "openvic-simulation/core/object/FixedPoint/Atomic.hpp"
#include "openvic-simulation/core/portable/ForwardableSpan.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/pop/PopNeedsMacro.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {
	struct BaseIssue;
	struct BuyResult;
	struct CountryInstance;
	struct CountryParty;
	struct Culture;
	struct DefineManager;
	struct GoodDefinition;
	struct Ideology;
	struct IssueManager;
	struct MarketInstance;
	struct PopDeps;
	struct PopManager;
	struct PopType;
	struct PopValuesFromProvince;
	struct ProvinceInstance;
	struct RebelType;
	struct Religion;
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

	#define OV_DO_FOR_ALL_TYPES_OF_POP_INCOME(F) \
		F(rgo_owner_income) \
		F(rgo_worker_income) \
		F(factory_worker_income) \
		F(factory_owner_income) \
		F(unemployment_subsidies) \
		F(pensions) \
		F(government_salary_administration) \
		F(government_salary_education) \
		F(government_salary_military) \
		F(event_and_decision_income) \
		F(loan_interest_payments)

	#define OV_DO_FOR_ALL_TYPES_OF_POP_EXPENSES(F) \
		F(life_needs_expense) \
		F(everyday_needs_expense) \
		F(luxury_needs_expense) \
		F(artisan_inputs_expense)

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop : PopBase {
		friend struct ProvinceInstance;

		enum struct culture_status_t : uint8_t {
			UNACCEPTED, ACCEPTED, PRIMARY
		};

		static constexpr bool is_culture_status_allowed(regiment_allowed_cultures_t allowed, culture_status_t status) {
			return static_cast<uint8_t>(allowed) <= static_cast<uint8_t>(status);
		}

		static constexpr pop_size_t MAX_SIZE = std::numeric_limits<pop_size_t>::max();

	private:
		std::optional<ArtisanalProducer> artisanal_producer_optional;
		fixed_point_t cash_allocated_for_artisanal_spending = 0;
		fixed_point_t artisanal_produce_left_to_sell = 0;
		pop_size_t employed = 0;

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
		OV_IFLATMAP_PROPERTY(Ideology, fixed_point_t, supporter_equivalents_by_ideology);
		memory::fixed_point_map_t<BaseIssue const*> PROPERTY(supporter_equivalents_by_issue);
		memory::fixed_point_map_t<CountryParty const*> PROPERTY(vote_equivalents_by_party);
	
	public:
		// The values returned by these functions are scaled by pop size, so they must be divided by pop size to get
		// the support as a proportion of 1.0
		fixed_point_t get_supporter_equivalents_by_issue(BaseIssue const& issue) const;
		fixed_point_t get_vote_equivalents_by_party(CountryParty const& party) const;

		static constexpr pop_size_t size_denominator = 200000;
		static constexpr size_t VECTORS_FOR_POP_TICK = 5;

		constexpr pop_size_t get_unemployed() const {
			return size - employed;
		}
		fixed_point_t get_unemployment_fraction() const;
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
				memory::fixed_point_map_t<GoodDefinition const*> PROPERTY(need_category##_needs); /* TODO pool? (if recalculating in UI is acceptable) */ \
				memory::ordered_map<GoodDefinition const*, bool> PROPERTY(need_category##_needs_fulfilled_goods);

		OV_DO_FOR_ALL_NEED_CATEGORIES(NEED_MEMBERS)
		#undef NEED_MEMBERS

		fixed_point_t PROPERTY(artisanal_revenue);
		#define DECLARE_POP_MONEY_STORES(money_type) \
			fixed_point_t PROPERTY(money_type);

		OV_DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_MONEY_STORES);
		OV_DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DECLARE_POP_MONEY_STORES);
		#undef DECLARE_POP_MONEY_STORES

		size_t PROPERTY(max_supported_regiments, 0);

		Pop(
			PopBase const& pop_base,
			decltype(supporter_equivalents_by_ideology)::keys_span_type ideology_keys,
			PopDeps const& pop_deps
		);

		memory::string get_pop_context_text() const;
		void reserve_needs_fulfilled_goods();
		void fill_needs_fulfilled_goods_with_false();
		void allocate_for_needs(
			memory::fixed_point_map_t<GoodDefinition const*> const& scaled_needs,
			forwardable_span<fixed_point_t> money_to_spend_per_good,
			memory::vector<fixed_point_t>& reusable_vector,
			fixed_point_t& price_inverse_sum,
			fixed_point_t& cash_left_to_spend
		);
		void pop_tick_without_cleanup(
			PopValuesFromProvince const& shared_values,
			RandomU32& random_number_generator,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			forwardable_span<
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

		void update_gamestate(
			DefineManager const& define_manager, CountryInstance const* owner,
			const fixed_point_t pop_size_per_regiment_multiplier
		);

		void add_artisanal_revenue(const fixed_point_t revenue);
		#define DECLARE_POP_MONEY_STORE_FUNCTIONS(name) \
			void add_##name(fixed_point_t amount);

		OV_DO_FOR_ALL_TYPES_OF_POP_INCOME(DECLARE_POP_MONEY_STORE_FUNCTIONS)
		OV_DO_FOR_ALL_TYPES_OF_POP_EXPENSES(DECLARE_POP_MONEY_STORE_FUNCTIONS)
		DECLARE_POP_MONEY_STORE_FUNCTIONS(import_subsidies)
		#undef DECLARE_POP_MONEY_STORE_FUNCTIONS

		void pop_tick(
			PopValuesFromProvince const& shared_values,
			RandomU32& random_number_generator,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_POP_TICK
			> reusable_vectors
		);
		void allocate_cash_for_artisanal_spending(const fixed_point_t money_to_spend);
		void report_artisanal_produce(const fixed_point_t quantity);
		void hire(pop_size_t count);
	};
}