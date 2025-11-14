#pragma once

#include <mutex>

#include "openvic-simulation/types/ClampedValue.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/Atomic.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/types/ValueHistory.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/reactive/DerivedState.hpp"

namespace OpenVic {
	struct BuyResult;
	struct CountryDefines;
	struct CountryInstance;
	struct CountryInstanceDeps;
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodInstance;
	struct MapInstance;
	struct MarketInstance;
	struct ModifierEffect;
	struct ModifierEffectCache;
	struct Pop;
	struct PopType;
	struct ProductionType;
	struct SellResult;
	struct SharedCountryValues;
	struct State;
	struct Strata;

	struct CountryEconomy {
		struct good_data_t {
			memory::unique_ptr<std::mutex> mutex;
			fixed_point_t stockpile_amount;
			fixed_point_t stockpile_change_yesterday; // positive if we gained, negative if we lost
			fixed_point_t quantity_traded_yesterday; // positive if we bought, negative if we sold
			fixed_point_t money_traded_yesterday; // positive if we sold, negative if we bought

			bool is_automated = true;
			bool is_selling = false; // buying if false
			fixed_point_t stockpile_cutoff;

			fixed_point_t exported_amount; // negative if net importing

			fixed_point_t government_needs;
			fixed_point_t army_needs;
			fixed_point_t navy_needs;
			fixed_point_t overseas_maintenance;
			fixed_point_t factory_demand;
			fixed_point_t pop_demand;
			fixed_point_t available_amount;

			ordered_map<PopType const*, fixed_point_t> need_consumption_per_pop_type;
			ordered_map<ProductionType const*, fixed_point_t> input_consumption_per_production_type;
			ordered_map<ProductionType const*, fixed_point_t> production_per_production_type;

			good_data_t();
			good_data_t(good_data_t&&) = default;
			good_data_t& operator=(good_data_t&&) = default;

			//thread safe
			void clear_daily_recorded_data();
		};

	private:
		OV_IFLATMAP_PROPERTY(GoodInstance, good_data_t, goods_data);
	public:
		good_data_t& get_good_data(GoodInstance const& good_instance);
		good_data_t const& get_good_data(GoodInstance const& good_instance) const;
		good_data_t& get_good_data(GoodDefinition const& good_definition);
		good_data_t const& get_good_data(GoodDefinition const& good_definition) const;

	private:
		CountryDefines const& country_defines;
		GameRulesManager const& game_rules_manager;
		MarketInstance& market_instance;
		ModifierEffectCache const& modifier_effect_cache;
		SharedCountryValues& shared_country_values;

		fixed_point_t cash_stockpile_start_of_tick;
		ValueHistory<fixed_point_t> PROPERTY(balance_history);
		OV_STATE_PROPERTY(fixed_point_t, gold_income);
		atomic_fixed_point_t PROPERTY(cash_stockpile);
		std::mutex taxable_income_mutex;
		OV_IFLATMAP_PROPERTY(PopType, fixed_point_t, taxable_income_by_pop_type);
		OV_STATE_PROPERTY(fixed_point_t, tax_efficiency);
		IndexedFlatMap<Strata, DerivedState<fixed_point_t>> PROPERTY(effective_tax_rate_by_strata);
	public:
		DerivedState<fixed_point_t>& get_effective_tax_rate_by_strata(Strata const& strata);
	private:
		IndexedFlatMap<Strata, ClampedValue> tax_rate_slider_value_by_strata;
	public:
		[[nodiscard]] constexpr IndexedFlatMap<Strata, ClampedValue> const& get_tax_rate_slider_value_by_strata() const {
			return tax_rate_slider_value_by_strata;
		}
		[[nodiscard]] ReadOnlyClampedValue& get_tax_rate_slider_value_by_strata(Strata const& strata);
		[[nodiscard]] ReadOnlyClampedValue const& get_tax_rate_slider_value_by_strata(Strata const& strata) const;
	private:
		OV_STATE_PROPERTY(fixed_point_t, administrative_efficiency_from_administrators);
		OV_STATE_PROPERTY(fixed_point_t, administrator_percentage);

		//store per slider per good: desired, bought & cost
		//store purchase record from last tick and prediction next tick
		OV_CLAMPED_PROPERTY(army_spending_slider_value);
		OV_CLAMPED_PROPERTY(navy_spending_slider_value);
		OV_CLAMPED_PROPERTY(construction_spending_slider_value);
		atomic_fixed_point_t PROPERTY(actual_national_stockpile_spending);
		atomic_fixed_point_t PROPERTY(actual_national_stockpile_income);

		OV_CLAMPED_PROPERTY(administration_spending_slider_value);
		OV_STATE_PROPERTY(fixed_point_t, projected_administration_spending_unscaled_by_slider);
		fixed_point_t actual_administration_budget;
		bool PROPERTY(was_administration_budget_cut_yesterday, false);
		atomic_fixed_point_t PROPERTY(actual_administration_spending);

		OV_CLAMPED_PROPERTY(education_spending_slider_value);
		OV_STATE_PROPERTY(fixed_point_t, projected_education_spending_unscaled_by_slider);
		fixed_point_t actual_education_budget;
		bool PROPERTY(was_education_budget_cut_yesterday, false);
		atomic_fixed_point_t PROPERTY(actual_education_spending);

		OV_CLAMPED_PROPERTY(military_spending_slider_value);
		OV_STATE_PROPERTY(fixed_point_t, projected_military_spending_unscaled_by_slider);
		fixed_point_t actual_military_budget;
		bool PROPERTY(was_military_budget_cut_yesterday, false);
		atomic_fixed_point_t PROPERTY(actual_military_spending);

		OV_CLAMPED_PROPERTY(social_spending_slider_value);
		OV_STATE_PROPERTY(fixed_point_t, projected_pensions_spending_unscaled_by_slider);
		OV_STATE_PROPERTY(fixed_point_t, projected_unemployment_subsidies_spending_unscaled_by_slider);
		fixed_point_t actual_social_budget;
		bool PROPERTY(was_social_budget_cut_yesterday, false);
		atomic_fixed_point_t PROPERTY(actual_pensions_spending);
		atomic_fixed_point_t PROPERTY(actual_unemployment_subsidies_spending);
		
		//base here means not scaled by slider or pop size
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> administration_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> education_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> military_salary_base_by_pop_type;
		IndexedFlatMap<PopType, DerivedState<fixed_point_t>> social_income_variant_base_by_pop_type;

		OV_CLAMPED_PROPERTY(tariff_rate_slider_value);
		fixed_point_t actual_import_subsidies_budget;
		bool PROPERTY(was_import_subsidies_budget_cut_yesterday, false);
		atomic_fixed_point_t actual_import_subsidies_spending;
		atomic_fixed_point_t actual_tariff_income;
		fixed_point_t get_actual_net_tariffs_balance() const {
			return actual_tariff_income.load() - actual_import_subsidies_spending.load();
		}

		//TODO actual factory subsidies
		//projected cost is UI only and lists the different factories
	public:
		static constexpr size_t VECTORS_FOR_COUNTRY_BUDGET_TICK = 4;

		DerivedState<fixed_point_t> corruption_cost_multiplier;
		DerivedState<fixed_point_t> tariff_efficiency;
		DerivedState<fixed_point_t> effective_tariff_rate;
		DerivedState<fixed_point_t> projected_administration_spending;
		DerivedState<fixed_point_t> projected_education_spending;
		DerivedState<fixed_point_t> projected_military_spending;
		DerivedState<fixed_point_t> projected_social_spending;
		DerivedState<fixed_point_t> projected_social_spending_unscaled_by_slider;
		DerivedState<fixed_point_t> projected_import_subsidies;
		DerivedState<fixed_point_t> projected_spending;
		DerivedState<bool> has_import_subsidies;

		CountryEconomy(
			SharedCountryValues& new_shared_country_values,
			CountryInstanceDeps const& country_instance_deps
		);
		
		fixed_point_t get_taxable_income_by_strata(Strata const& strata) const;

		void set_strata_tax_rate_slider_value(Strata const& strata, const fixed_point_t new_value);
		void set_army_spending_slider_value(const fixed_point_t new_value);
		void set_navy_spending_slider_value(const fixed_point_t new_value);
		void set_construction_spending_slider_value(const fixed_point_t new_value);
		void set_education_spending_slider_value(const fixed_point_t new_value);
		void set_administration_spending_slider_value(const fixed_point_t new_value);
		void set_social_spending_slider_value(const fixed_point_t new_value);
		void set_military_spending_slider_value(const fixed_point_t new_value);
		void set_tariff_rate_slider_value(const fixed_point_t new_value);

		void update_country_budget(
			const fixed_point_t desired_administrator_percentage,
			IndexedFlatMap<PopType, pop_size_t> const& population_by_type,
			IndexedFlatMap<PopType, pop_size_t> const& unemployed_pops_by_type,
			ordered_set<State*> const& states
		);
		void country_budget_tick_before_map(
			CountryInstance const& country_instance,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			utility::forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_COUNTRY_BUDGET_TICK
			> reusable_vectors,
			memory::vector<size_t>& reusable_index_vector
		);
		void country_budget_tick_after_map(
			CountryInstance const& country_instance,
			const Date today
		);
		
		//thread safe
		void report_pop_income_tax(PopType const& pop_type, const fixed_point_t gross_income, const fixed_point_t paid_as_tax);
		void report_pop_need_consumption(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_pop_need_demand(PopType const& pop_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_input_consumption(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_input_demand(ProductionType const& production_type, GoodDefinition const& good, const fixed_point_t quantity);
		void report_output(ProductionType const& production_type, const fixed_point_t quantity);
		void request_salaries_and_welfare_and_import_subsidies(Pop& pop);
		fixed_point_t calculate_minimum_wage_base(PopType const& pop_type);
		fixed_point_t apply_tariff(const fixed_point_t money_spent_on_imports);
	
	private:
		static void after_buy(void* actor, BuyResult const& buy_result);
		//matching GoodMarketSellOrder::callback_t
		static void after_sell(void* actor, SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector);

		void manage_national_stockpile(
			CountryInstance const& country_instance,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			utility::forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_COUNTRY_BUDGET_TICK
			> reusable_vectors,
			memory::vector<size_t>& reusable_index_vector,
			const fixed_point_t available_funds
		);
		
		//base here means not scaled by slider or pop size
		fixed_point_t calculate_pensions_base(PopType const& pop_type);
		fixed_point_t calculate_unemployment_subsidies_base(PopType const& pop_type);
		
		virtual fixed_point_t get_modifier_effect_value(ModifierEffect const& effect) const = 0;
		virtual fixed_point_t get_yesterdays_import_value(DependencyTracker& tracker) = 0;
	};
}