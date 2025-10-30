#include "GameRulesManager.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"

using namespace OpenVic;

bool GameRulesManager::may_use_coastal_artisanal_production_types(ProvinceInstance const& province) const {
	const bool is_coastal_province = province.get_province_definition().is_coastal();
	using enum artisan_coastal_restriction_t;
	switch (coastal_restriction_for_artisans) {
		case Unrestricted:
			return true;
		case CountriesWithCoast: {
			CountryInstance const* const country = province.get_country_to_report_economy();
			return is_coastal_province || (country != nullptr && country->is_coastal());
		}
		case CoastalStates: {
			State const* const state = province.get_state();
			return is_coastal_province || (state != nullptr && state->is_coastal());
		}
		case CoastalProvinces:
			return is_coastal_province;
	}
}

bool GameRulesManager::may_use_coastal_factory_production_types(State const& state) const {
	const bool is_coastal_state = state.is_coastal();
	using enum factory_coastal_restriction_t;
	switch (coastal_restriction_for_factories) {
		case Unrestricted:
			return true;
		case CountriesWithCoast: {
			CountryInstance const* const country = state.get_owner();
			return is_coastal_state || (country != nullptr && country->is_coastal());
		}
		case CoastalStates: {
			return is_coastal_state;
		}
	}
}