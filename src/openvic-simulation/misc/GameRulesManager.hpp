#pragma once

#include <cstdint>

#include "openvic-simulation/utility/Getters.hpp"

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
		bool PROPERTY_RW(use_simple_farm_mine_logic, false);
		// if changed during a session, call on_use_exponential_price_changes_changed for each GoodInstance.
		bool PROPERTY_RW(use_exponential_price_changes, false);
		bool PROPERTY_RW(prevent_negative_administration_efficiency, false);
		demand_category PROPERTY_RW(artisanal_input_demand_category, demand_category::None);
		country_to_report_economy_t PROPERTY_RW(country_to_report_economy, country_to_report_economy_t::Owner);
		artisan_coastal_restriction_t PROPERTY_RW(
			coastal_restriction_for_artisans,
			artisan_coastal_restriction_t::CountriesWithCoast
		);
		factory_coastal_restriction_t PROPERTY_RW(
			coastal_restriction_for_factories,
			factory_coastal_restriction_t::CoastalStates
		);

	public:
		constexpr bool get_use_optimal_pricing() const {
			return use_exponential_price_changes;
		}

		bool may_use_coastal_artisanal_production_types(ProvinceInstance const& province) const;
		bool may_use_coastal_factory_production_types(State const& state) const;
	};
}