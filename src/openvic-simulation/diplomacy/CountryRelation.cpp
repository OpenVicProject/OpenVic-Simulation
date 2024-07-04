#include "CountryRelation.hpp"

#include <cassert>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;

CountryRelationInstanceProxy::CountryRelationInstanceProxy(std::string_view id) : country_id { id } {}

CountryRelationInstanceProxy::CountryRelationInstanceProxy(CountryInstance const* country)
	: country_id { country->get_country_definition()->get_identifier() } {}

CountryRelationInstanceProxy::operator std::string_view() const {
	return country_id;
}

CountryRelationManager::CountryRelationManager(/* TODO: Country Instance Manager Reference */) {}

bool CountryRelationManager::add_country(CountryRelationInstanceProxy country) {
	// TODO: iterate through Country Instances adding country to every previously existing country_relations
	return true;
}

bool CountryRelationManager::remove_country(CountryRelationInstanceProxy country) {
	// TODO: iterate through country_relations removing every pair that references the country's id
	return true;
}

country_relation_value_t CountryRelationManager::get_country_relation(
	CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient
) const {
	auto it = country_relations.find({ country.country_id, recepient.country_id });
	OV_ERR_FAIL_COND_V(it == country_relations.end(), 0);
	return it->second;
}

country_relation_value_t*
CountryRelationManager::get_country_relation_ptr(CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient) {
	auto it = country_relations.find({ country.country_id, recepient.country_id });
	OV_ERR_FAIL_COND_V(it == country_relations.end(), nullptr);
	return &it.value();
}

bool CountryRelationManager::set_country_relation(
	CountryRelationInstanceProxy country, CountryRelationInstanceProxy recepient, country_relation_value_t value
) {
	auto it = country_relations.find({ country.country_id, recepient.country_id });
	OV_ERR_FAIL_COND_V(it == country_relations.end(), false);
	it.value() = value;
	return true;
}
