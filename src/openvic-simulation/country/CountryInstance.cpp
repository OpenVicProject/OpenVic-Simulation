#include "CountryInstance.hpp"

using namespace OpenVic;

bool CountryInstance::add_accepted_culture(Culture const* new_accepted_culture) {
	if(std::find(accepted_cultures.begin(), accepted_cultures.end(), new_accepted_culture) != accepted_cultures.end()) {
		Logger::warning("Attempted to add accepted culture ", new_accepted_culture->get_identifier(), " to country ", base_country->get_identifier(), ": already present!");
		return false;
	}
	accepted_cultures.push_back(new_accepted_culture);
	return true;
}

bool CountryInstance::remove_accepted_culture(Culture const* culture_to_remove) {
	auto existing_entry = std::find(accepted_cultures.begin(), accepted_cultures.end(), culture_to_remove);
	if (existing_entry == accepted_cultures.end()) {
		Logger::warning("Attempted to remove accepted culture ", culture_to_remove->get_identifier(), " from country ", base_country->get_identifier(), ": not present!");
		return false;
	}
	accepted_cultures.erase(existing_entry);
	return true;
}

void CountryInstance::add_to_upper_house(Ideology const* party, fixed_point_t popularity) {
	upper_house[party] = popularity;
}

bool CountryInstance::remove_from_upper_house(Ideology const* party) {
	return upper_house.erase(party) == 1;
}

bool CountryInstance::add_reform(Reform const* new_reform) {
	if (std::find(reforms.begin(), reforms.end(), new_reform) != reforms.end()) {
		Logger::warning("Attempted to add reform ", new_reform->get_identifier(), " to country ", base_country->get_identifier(), ": already present!");
		return false;
	}
	reforms.push_back(new_reform);
	return true;
}

bool CountryInstance::remove_reform(Reform const* reform_to_remove) {
	auto existing_entry = std::find(reforms.begin(), reforms.end(), reform_to_remove);
	if (existing_entry == reforms.end()) {
		Logger::warning("Attempted to remove reform ", reform_to_remove->get_identifier(), " from country ", base_country->get_identifier(), ": not present!");
		return false;
	}
	reforms.erase(existing_entry);
	return true;
}

bool CountryInstance::apply_history_to_country(CountryHistoryMap const& history, Date date) {
	accepted_cultures.clear();
	upper_house.clear();
	reforms.clear();

	bool ret = true;
	for (CountryHistoryEntry const* entry : history.get_entries_up_to(date)) {
		if (entry->get_primary_culture()) primary_culture = *entry->get_primary_culture();
		for (Culture const* culture : entry->get_accepted_cultures()) {
			ret &= add_accepted_culture(culture);
		}
		if (entry->get_religion()) religion = *entry->get_religion();
		if (entry->get_ruling_party()) ruling_party = *entry->get_ruling_party();
		if (entry->get_last_election()) last_election = *entry->get_last_election();
		for (auto const& [ideology, popularity] : entry->get_upper_house()) {
			add_to_upper_house(ideology, popularity);
		}
		if (entry->get_capital()) capital = *entry->get_capital();
		if (entry->get_government_type()) government_type = *entry->get_government_type();
		if (entry->get_plurality()) plurality = *entry->get_plurality();
		if (entry->get_national_value()) national_value = *entry->get_national_value();
		if (entry->get_civilised()) civilised = *entry->get_civilised();
		if (entry->get_prestige()) prestige = *entry->get_prestige();
		for (Reform const* reform : entry->get_reforms()) {
			ret &= add_reform(reform);
		}
	}
	return ret;
}
