#include "CountryRelation.hpp"

#include <cassert>

#include "openvic-simulation/country/CountryInstance.hpp"

using namespace OpenVic;
using CRM = CountryRelationManager;
using relation_value_type = OpenVic::CountryRelationManager::relation_value_type;
using influence_value_type = OpenVic::CountryRelationManager::influence_value_type;
using OpinionType = OpenVic::CountryRelationManager::OpinionType;

relation_value_type CRM::get_country_relation( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(relations)::const_iterator it = relations.find({ country, recipient });
	if (it == relations.end()) {
		return {};
	}
	return it->second;
}

relation_value_type& CRM::assign_or_get_country_relation( //
	CountryInstance* country, CountryInstance* recipient, relation_value_type default_value
) {
	decltype(relations)::iterator it = relations.find({ country, recipient });
	if (it == relations.end()) {
		it = relations.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_relation( //
	CountryInstance* country, CountryInstance* recipient, relation_value_type value
) {
	decltype(relations)::iterator it = relations.find({ country, recipient });
	if (it == relations.end()) {
		it = relations.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::relations)::values_container_type const& CRM::get_country_relation_values() const {
	return relations.values_container();
}

bool CRM::get_country_alliance( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(alliances)::const_iterator it = alliances.find({ country, recipient });
	if (it == alliances.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_country_alliance( //
	CountryInstance* country, CountryInstance* recipient, bool default_value
) {
	decltype(alliances)::iterator it = alliances.find({ country, recipient });
	if (it == alliances.end()) {
		it = alliances.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_alliance( //
	CountryInstance* country, CountryInstance* recipient, bool value
) {
	decltype(alliances)::iterator it = alliances.find({ country, recipient });
	if (it == alliances.end()) {
		it = alliances.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::alliances)::values_container_type const& CRM::get_country_alliance_values() const {
	return alliances.values_container();
}

bool CRM::get_at_war_with( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(at_war)::const_iterator it = at_war.find({ country, recipient });
	if (it == at_war.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_at_war_with( //
	CountryInstance* country, CountryInstance* recipient, bool default_value
) {
	decltype(at_war)::iterator it = at_war.find({ country, recipient });
	if (it == at_war.end()) {
		it = at_war.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_at_war_with( //
	CountryInstance* country, CountryInstance* recipient, bool value
) {
	decltype(at_war)::iterator it = at_war.find({ country, recipient });
	if (it == at_war.end()) {
		it = at_war.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	it.value() = value;
	return true;
}

decltype(CRM::at_war)::values_container_type const& CRM::get_at_war_with_values() const {
	return at_war.values_container();
}

bool CRM::get_has_military_access_to( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(military_access)::const_iterator it = military_access.find({ country, recipient });
	if (it == military_access.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_has_military_access_to( //
	CountryInstance* country, CountryInstance const* recipient, bool default_value
) {
	decltype(military_access)::iterator it = military_access.find({ country, recipient });
	if (it == military_access.end()) {
		it = military_access.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_has_military_access_to( //
	CountryInstance* country, CountryInstance const* recipient, bool value
) {
	decltype(military_access)::iterator it = military_access.find({ country, recipient });
	if (it == military_access.end()) {
		it = military_access.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::military_access)::values_container_type const& CRM::get_has_military_access_to_values() const {
	return military_access.values_container();
}

bool CRM::get_war_subsidies_to( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(war_subsidies)::const_iterator it = war_subsidies.find({ country, recipient });
	if (it == war_subsidies.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_war_subsidies_to( //
	CountryInstance* country, CountryInstance const* recipient, bool default_value
) {
	decltype(war_subsidies)::iterator it = war_subsidies.find({ country, recipient });
	if (it == war_subsidies.end()) {
		it = war_subsidies.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_war_subsidies_to( //
	CountryInstance* country, CountryInstance const* recipient, bool value
) {
	decltype(war_subsidies)::iterator it = war_subsidies.find({ country, recipient });
	if (it == war_subsidies.end()) {
		it = war_subsidies.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::war_subsidies)::values_container_type const& CRM::get_war_subsidies_to_values() const {
	return war_subsidies.values_container();
}

bool CRM::get_commands_units( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(command_units)::const_iterator it = command_units.find({ country, recipient });
	if (it == command_units.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_commands_units( //
	CountryInstance* country, CountryInstance const* recipient, bool default_value
) {
	decltype(command_units)::iterator it = command_units.find({ country, recipient });
	if (it == command_units.end()) {
		it = command_units.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_commands_units( //
	CountryInstance* country, CountryInstance const* recipient, bool value
) {
	decltype(command_units)::iterator it = command_units.find({ country, recipient });
	if (it == command_units.end()) {
		it = command_units.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::command_units)::values_container_type const& CRM::get_commands_units_values() const {
	return command_units.values_container();
}

bool CRM::get_has_vision( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(vision)::const_iterator it = vision.find({ country, recipient });
	if (it == vision.end()) {
		return {};
	}
	return it->second;
}

bool& CRM::assign_or_get_has_vision( //
	CountryInstance* country, CountryInstance const* recipient, bool default_value
) {
	decltype(vision)::iterator it = vision.find({ country, recipient });
	if (it == vision.end()) {
		it = vision.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_has_vision( //
	CountryInstance* country, CountryInstance const* recipient, bool value
) {
	decltype(vision)::iterator it = vision.find({ country, recipient });
	if (it == vision.end()) {
		it = vision.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::vision)::values_container_type const& CRM::get_has_vision_values() const {
	return vision.values_container();
}

OpinionType CRM::get_country_opinion( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(opinions)::const_iterator it = opinions.find({ country, recipient });
	if (it == opinions.end()) {
		return {};
	}
	return it->second;
}

OpinionType& CRM::assign_or_get_country_opinion( //
	CountryInstance* country, CountryInstance const* recipient, OpinionType default_value
) {
	decltype(opinions)::iterator it = opinions.find({ country, recipient });
	if (it == opinions.end()) {
		it = opinions.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_country_opinion( //
	CountryInstance* country, CountryInstance const* recipient, OpinionType value
) {
	decltype(opinions)::iterator it = opinions.find({ country, recipient });
	if (it == opinions.end()) {
		it = opinions.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::opinions)::values_container_type const& CRM::get_country_opinion_values() const {
	return opinions.values_container();
}

influence_value_type CRM::get_influence_with( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(influence)::const_iterator it = influence.find({ country, recipient });
	if (it == influence.end()) {
		return {};
	}
	return it->second;
}

influence_value_type& CRM::assign_or_get_influence_with(
	CountryInstance* country, CountryInstance const* recipient, influence_value_type default_value
) {
	decltype(influence)::iterator it = influence.find({ country, recipient });
	if (it == influence.end()) {
		it = influence.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_influence_with(CountryInstance* country, CountryInstance const* recipient, influence_value_type value) {
	decltype(influence)::iterator it = influence.find({ country, recipient });
	if (it == influence.end()) {
		it = influence.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::influence)::values_container_type const& CRM::get_influence_with_values() const {
	return influence.values_container();
}

std::optional<Date> CRM::get_discredited_date( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(discredits)::const_iterator it = discredits.find({ country, recipient });
	if (it == discredits.end()) {
		return {};
	}
	return it->second;
}

Date& CRM::assign_or_get_discredited_date( //
	CountryInstance* country, CountryInstance const* recipient, Date default_value
) {
	decltype(discredits)::iterator it = discredits.find({ country, recipient });
	if (it == discredits.end()) {
		it = discredits.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_discredited_date( //
	CountryInstance* country, CountryInstance const* recipient, Date value
) {
	decltype(discredits)::iterator it = discredits.find({ country, recipient });
	if (it == discredits.end()) {
		it = discredits.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::discredits.values_container()) CRM::get_discredited_date_values() const {
	return discredits.values_container();
}

std::optional<Date> CRM::get_embassy_banned_date( //
	CountryInstance const* country, CountryInstance const* recipient
) const {
	decltype(embassy_bans)::const_iterator it = embassy_bans.find({ country, recipient });
	if (it == embassy_bans.end()) {
		return {};
	}
	return it->second;
}

Date& CRM::assign_or_get_embassy_banned_date( //
	CountryInstance* country, CountryInstance const* recipient, Date default_value
) {
	decltype(embassy_bans)::iterator it = embassy_bans.find({ country, recipient });
	if (it == embassy_bans.end()) {
		it = embassy_bans.insert({ { country, recipient }, default_value }).first;
	}
	return it.value();
}

bool CRM::set_embassy_banned_date( //
	CountryInstance* country, CountryInstance const* recipient, Date value
) {
	decltype(embassy_bans)::iterator it = embassy_bans.find({ country, recipient });
	if (it == embassy_bans.end()) {
		it = embassy_bans.insert({ { country, recipient }, value }).first;
	} else {
		it.value() = value;
	}
	return true;
}

decltype(CRM::embassy_bans.values_container()) CRM::get_embassy_banned_date_values() const {
	return embassy_bans.values_container();
}
