#include "CountryInstance.hpp"

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"

using namespace OpenVic;

CountryInstance::CountryInstance(CountryDefinition const* new_country_definition)
  : country_definition { new_country_definition }, primary_culture { nullptr }, religion { nullptr }, ruling_party { nullptr },
	last_election {}, capital { nullptr }, government_type { nullptr }, plurality { 0 }, national_value { nullptr },
	civilised { false }, prestige { 0 } {}

std::string_view CountryInstance::get_identifier() const {
	return country_definition != nullptr ? country_definition->get_identifier() : "NULL";
}

bool CountryInstance::add_accepted_culture(Culture const* new_accepted_culture) {
	if (std::find(accepted_cultures.begin(), accepted_cultures.end(), new_accepted_culture) != accepted_cultures.end()) {
		Logger::warning(
			"Attempted to add accepted culture ", new_accepted_culture->get_identifier(), " to country ",
			country_definition->get_identifier(), ": already present!"
		);
		return false;
	}
	accepted_cultures.push_back(new_accepted_culture);
	return true;
}

bool CountryInstance::remove_accepted_culture(Culture const* culture_to_remove) {
	auto existing_entry = std::find(accepted_cultures.begin(), accepted_cultures.end(), culture_to_remove);
	if (existing_entry == accepted_cultures.end()) {
		Logger::warning(
			"Attempted to remove accepted culture ", culture_to_remove->get_identifier(), " from country ",
			country_definition->get_identifier(), ": not present!"
		);
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
		Logger::warning(
			"Attempted to add reform ", new_reform->get_identifier(), " to country ", country_definition->get_identifier(),
			": already present!"
		);
		return false;
	}
	reforms.push_back(new_reform);
	return true;
}

bool CountryInstance::remove_reform(Reform const* reform_to_remove) {
	auto existing_entry = std::find(reforms.begin(), reforms.end(), reform_to_remove);
	if (existing_entry == reforms.end()) {
		Logger::warning(
			"Attempted to remove reform ", reform_to_remove->get_identifier(), " from country ",
			country_definition->get_identifier(), ": not present!"
		);
		return false;
	}
	reforms.erase(existing_entry);
	return true;
}

bool CountryInstance::apply_history_to_country(CountryHistoryEntry const* entry) {
	if (entry == nullptr) {
		Logger::error("Trying to apply null country history to ", get_identifier());
		return false;
	}

	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
		}
	};

	bool ret = true;

	set_optional(primary_culture, entry->get_primary_culture());
	for (Culture const* culture : entry->get_accepted_cultures()) {
		ret &= add_accepted_culture(culture);
	}
	set_optional(religion, entry->get_religion());
	set_optional(ruling_party, entry->get_ruling_party());
	set_optional(last_election, entry->get_last_election());
	for (auto const& [ideology, popularity] : entry->get_upper_house()) {
		add_to_upper_house(ideology, popularity);
	}
	set_optional(capital, entry->get_capital());
	set_optional(government_type, entry->get_government_type());
	set_optional(plurality, entry->get_plurality());
	set_optional(national_value, entry->get_national_value());
	set_optional(civilised, entry->is_civilised());
	set_optional(prestige, entry->get_prestige());
	for (Reform const* reform : entry->get_reforms()) {
		ret &= add_reform(reform);
	}

	return ret;
}

bool CountryInstanceManager::generate_country_instances(CountryDefinitionManager const& country_definition_manager) {
	reserve_more(country_instances, country_definition_manager.get_country_definition_count());

	for (CountryDefinition const& country_definition : country_definition_manager.get_country_definitions()) {
		country_instances.add_item({ &country_definition });
	}

	return true;
}

bool CountryInstanceManager::apply_history_to_countries(
	CountryHistoryManager const& history_manager, Date date, UnitInstanceManager& unit_instance_manager,
	MapInstance& map_instance
) {
	bool ret = true;

	for (CountryInstance& country_instance : country_instances.get_items()) {
		if (!country_instance.get_country_definition()->is_dynamic_tag()) {
			CountryHistoryMap const* history_map =
				history_manager.get_country_history(country_instance.get_country_definition());

			if (history_map != nullptr) {
				CountryHistoryEntry const* oob_history_entry = nullptr;

				for (CountryHistoryEntry const* entry : history_map->get_entries_up_to(date)) {
					country_instance.apply_history_to_country(entry);

					if (entry->get_inital_oob()) {
						oob_history_entry = entry;
					}
				}

				if (oob_history_entry != nullptr) {
					unit_instance_manager.generate_deployment(
						map_instance, country_instance, *oob_history_entry->get_inital_oob()
					);
				}
			}
		}
	}

	return ret;
}
