#pragma once

#include <cstdint>

#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

namespace OpenVic {
	struct ProvinceInstance;
	struct State;

	enum struct country_to_report_economy_t : uint8_t {
		Owner,
		Controller,
		NeitherWhenOccupied
	};

	enum struct demand_category : uint8_t {
		None,
		PopNeeds,
		FactoryNeeds
	};

	enum struct artisan_coastal_restriction_t : uint8_t {
		Unrestricted,
		CountriesWithCoast,
		CoastalStates,
		CoastalProvinces
	};

	enum struct factory_coastal_restriction_t : uint8_t {
		Unrestricted,
		CountriesWithCoast,
		CoastalStates
	};

	struct GameRulesManager {
	private:
		bool PROPERTY_RW(use_simple_farm_mine_logic);
		// if changed during a session, call on_use_exponential_price_changes_changed for each GoodInstance.
		bool PROPERTY_RW(use_exponential_price_changes);
		bool PROPERTY_RW(prevent_negative_administration_efficiency);
		bool PROPERTY_RW(should_artisans_discard_unsold_non_inputs);
		demand_category PROPERTY_RW(artisanal_input_demand_category);
		country_to_report_economy_t PROPERTY_RW(country_to_report_economy);
		artisan_coastal_restriction_t PROPERTY_RW(coastal_restriction_for_artisans);
		factory_coastal_restriction_t PROPERTY_RW(coastal_restriction_for_factories);

	public:
		constexpr bool get_use_optimal_pricing() const {
			return use_exponential_price_changes;
		}

		constexpr GameRulesManager() { // NOLINT(cppcoreguidelines-pro-type-member-init)
			use_victoria_2_rules();
		}

		bool may_use_coastal_artisanal_production_types(ProvinceInstance const& province) const;
		bool may_use_coastal_factory_production_types(State const& state) const;

		OV_ALWAYS_INLINE constexpr void use_recommended_rules() {
			use_simple_farm_mine_logic = true;
			use_exponential_price_changes = true;
			prevent_negative_administration_efficiency = true;
			should_artisans_discard_unsold_non_inputs = false;
			artisanal_input_demand_category = demand_category::FactoryNeeds;
			country_to_report_economy = country_to_report_economy_t::Controller;
			coastal_restriction_for_artisans = artisan_coastal_restriction_t::CoastalProvinces;
			coastal_restriction_for_factories = factory_coastal_restriction_t::CoastalStates;
		}

		OV_ALWAYS_INLINE constexpr void use_victoria_2_rules() {
			use_simple_farm_mine_logic = false;
			use_exponential_price_changes = false;
			prevent_negative_administration_efficiency = false;
			should_artisans_discard_unsold_non_inputs = true;
			artisanal_input_demand_category = demand_category::None;
			country_to_report_economy = country_to_report_economy_t::Owner;
			coastal_restriction_for_artisans = artisan_coastal_restriction_t::CountriesWithCoast;
			coastal_restriction_for_factories = factory_coastal_restriction_t::CoastalStates;
		}
	};
}