#include "CountryRelation.hpp"

#include <cassert>

#include "openvic-simulation/country/CountryInstance.hpp"

using namespace OpenVic;
using CRM = CountryRelationManager;
using relation_value_type = OpenVic::CountryRelationManager::relation_value_type;
using influence_value_type = OpenVic::CountryRelationManager::influence_value_type;
using OpinionType = OpenVic::CountryRelationManager::OpinionType;

relation_value_type CRM::get_country_relation(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(relations)::const_iterator it = relations.find({ country, recepient });
	if (it == relations.end()) {
		return {};
	}
	return it->second;
}

relation_value_type& CRM::assign_or_get_country_relation(
	CountryInstance* country, CountryInstance* recepient, relation_value_type default_value
) {
	decltype(relations)::iterator it = relations.find({ country, recepient });
	if (it == relations.end()) {
		it = relations.insert({ { country, recepient }, default_value }).first;
		country->get_relations().insert_or_assign(recepient, &it.value());
	}
	return it.value();
}

bool CRM::set_country_relation(
	CountryInstance* country, CountryInstance* recepient, relation_value_type value
) {
	decltype(relations)::iterator it = relations.find({ country, recepient });
	if (it == relations.end()) {
		it = relations.insert({ { country, recepient }, value }).first;
		country->get_relations().insert_or_assign(recepient, &it.value());
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::relations)::values_container_type const& CRM::get_country_relation_values() const {
	return relations.values_container();
}

bool CRM::get_country_alliance(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(alliances)::const_iterator it = alliances.find({ country, recepient });
	if (it == alliances.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_alliance(
	CountryInstance* country, CountryInstance* recepient, bool default_value
) {
	decltype(alliances)::iterator it = alliances.find({ country, recepient });
	if (it == alliances.end()) {
		it = alliances.insert({ { country, recepient }, default_value }).first;
		country->get_alliances().insert_or_assign(recepient, &it.value());
	}
	return it.value();
}

bool CRM::set_country_alliance(
	CountryInstance* country, CountryInstance* recepient, bool value
) {
	decltype(alliances)::iterator it = alliances.find({ country, recepient });
	if (it == alliances.end()) {
		it = alliances.insert({ { country, recepient }, value }).first;
		country->get_alliances().insert_or_assign(recepient, &it.value());
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::alliances)::values_container_type const& CRM::get_country_alliance_values() const {
	return alliances.values_container();
}

bool CRM::get_country_at_war(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(at_war)::const_iterator it = at_war.find({ country, recepient });
	if (it == at_war.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_at_war(
	CountryInstance* country, CountryInstance* recepient, bool default_value
) {
	decltype(at_war)::iterator it = at_war.find({ country, recepient });
	if (it == at_war.end()) {
		it = at_war.insert({ { country, recepient }, default_value }).first;
		country->get_at_war_with().insert_or_assign(recepient, &it.value());
	}
	return it.value();
}

bool CRM::set_country_at_war(
	CountryInstance* country, CountryInstance* recepient, bool value
) {
	decltype(at_war)::iterator it = at_war.find({ country, recepient });
	if (it == at_war.end()) {
		it = at_war.insert({ { country, recepient }, value }).first;
		country->get_at_war_with().insert_or_assign(recepient, &it.value());
	} else {
		it.value() = value;
	}
	it.value() = value;
	return true;
}

decltype(CRM::at_war)::values_container_type const& CRM::get_country_at_war_values() const {
	return at_war.values_container();
}

bool CRM::get_country_military_access(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(military_access)::const_iterator it = military_access.find({ country, recepient });
	if (it == military_access.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_military_access(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(military_access)::iterator it = military_access.find({ country, recepient });
	if (it == military_access.end()) {
		it = military_access.insert({ { country, recepient }, default_value }).first;
		country->get_military_access().insert_or_assign(recepient, &it.value());
	}
	return it.value();
}

bool CRM::set_country_military_access(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(military_access)::iterator it = military_access.find({ country, recepient });
	if (it == military_access.end()) {
		it = military_access.insert({ { country, recepient }, value }).first;
		country->get_military_access().insert_or_assign(recepient, &it.value());
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::military_access)::values_container_type const& CRM::get_country_military_access_values() const {
	return military_access.values_container();
}

bool CRM::get_country_war_subsidies(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(war_subsidies)::const_iterator it = war_subsidies.find({ country, recepient });
	if (it == war_subsidies.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_war_subsidies(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(war_subsidies)::iterator it = war_subsidies.find({ country, recepient });
	if (it == war_subsidies.end()) {
		it = war_subsidies.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_war_subsidies(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(war_subsidies)::iterator it = war_subsidies.find({ country, recepient });
	if (it == war_subsidies.end()) {
		it = war_subsidies.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::war_subsidies)::values_container_type const& CRM::get_country_war_subsidies_values() const {
	return war_subsidies.values_container();
}

bool CRM::get_country_command_units(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(command_units)::const_iterator it = command_units.find({ country, recepient });
	if (it == command_units.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_command_units(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(command_units)::iterator it = command_units.find({ country, recepient });
	if (it == command_units.end()) {
		it = command_units.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_command_units(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(command_units)::iterator it = command_units.find({ country, recepient });
	if (it == command_units.end()) {
		it = command_units.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::command_units)::values_container_type const& CRM::get_country_command_units_values() const {
	return command_units.values_container();
}

bool CRM::get_country_vision(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(vision)::const_iterator it = vision.find({ country, recepient });
	if (it == vision.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_vision(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(vision)::iterator it = vision.find({ country, recepient });
	if (it == vision.end()) {
		it = vision.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_vision(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(vision)::iterator it = vision.find({ country, recepient });
	if (it == vision.end()) {
		it = vision.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::vision)::values_container_type const& CRM::get_country_vision_values() const {
	return vision.values_container();
}

OpinionType CRM::get_country_opinion(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(opinions)::const_iterator it = opinions.find({ country, recepient });
	if (it == opinions.end()) {
		return {};
	}
	return it->second;
}

OpinionType& CRM::assign_or_get_country_opinion(
	CountryInstance* country, CountryInstance const* recepient, OpinionType default_value
) {
	decltype(opinions)::iterator it = opinions.find({ country, recepient });
	if (it == opinions.end()) {
		it = opinions.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_opinion(
	CountryInstance* country, CountryInstance const* recepient, OpinionType value
) {
	decltype(opinions)::iterator it = opinions.find({ country, recepient });
	if (it == opinions.end()) {
		it = opinions.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::opinions)::values_container_type const& CRM::get_country_opinion_values() const {
	return opinions.values_container();
}

influence_value_type CRM::get_country_influence(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(influence)::const_iterator it = influence.find({ country, recepient });
	if (it == influence.end()) {
		return {};
	}
	return it->second;
}

influence_value_type& CRM::assign_or_get_country_influence(
	CountryInstance* country, CountryInstance const* recepient, influence_value_type default_value
) {
	decltype(influence)::iterator it = influence.find({ country, recepient });
	if (it == influence.end()) {
		it = influence.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_influence(CountryInstance* country, CountryInstance const* recepient, influence_value_type value) {
	decltype(influence)::iterator it = influence.find({ country, recepient });
	if (it == influence.end()) {
		it = influence.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::influence)::values_container_type const& CRM::get_country_influence_values() const {
	return influence.values_container();
}

bool CRM::get_country_discredit(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(discredits)::const_iterator it = discredits.find({ country, recepient });
	if (it == discredits.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_discredit(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(discredits)::iterator it = discredits.find({ country, recepient });
	if (it == discredits.end()) {
		it = discredits.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_discredit(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(discredits)::iterator it = discredits.find({ country, recepient });
	if (it == discredits.end()) {
		it = discredits.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::discredits)::values_container_type const& CRM::get_country_discredit_values() const {
	return discredits.values_container();
}

bool CRM::get_country_embassy_ban(
	CountryInstance const* country, CountryInstance const* recepient
) const {
	decltype(embassy_bans)::const_iterator it = embassy_bans.find({ country, recepient });
	if (it == embassy_bans.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_embassy_ban(
	CountryInstance* country, CountryInstance const* recepient, bool default_value
) {
	decltype(embassy_bans)::iterator it = embassy_bans.find({ country, recepient });
	if (it == embassy_bans.end()) {
		it = embassy_bans.insert({ { country, recepient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_embassy_ban(
	CountryInstance* country, CountryInstance const* recepient, bool value
) {
	decltype(embassy_bans)::iterator it = embassy_bans.find({ country, recepient });
	if (it == embassy_bans.end()) {
		it = embassy_bans.insert({ { country, recepient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::embassy_bans)::values_container_type const& CRM::get_country_embassy_ban_values() const {
	return embassy_bans.values_container();
}
