#include "PoliticsInstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"

using namespace OpenVic;

PoliticsInstanceManager::PoliticsInstanceManager(InstanceManager const& new_instance_manager)
  : instance_manager { new_instance_manager },
	politics_manager { instance_manager.get_definition_manager().get_politics_manager() },
	ideology_spawn_date { politics_manager.get_ideology_manager().get_ideologies() } {}

void PoliticsInstanceManager::setup_starting_ideologies() {
	const Date today = instance_manager.get_today();

	for (auto [ideology, spawn_date] : ideology_spawn_date) {
		std::optional<Date> const& defines_spawn_date = ideology.get_spawn_date();

		if (!defines_spawn_date.has_value()) {
			spawn_date = today;
		} else if (*defines_spawn_date <= today) {
			spawn_date = defines_spawn_date;
		}
	}
}

std::optional<Date> const& PoliticsInstanceManager::get_ideology_spawn_date(Ideology const& ideology) const {
	return ideology_spawn_date[ideology];
}

bool PoliticsInstanceManager::is_ideology_unlocked(Ideology const& ideology) const {
	return get_ideology_spawn_date(ideology).has_value();
}

void PoliticsInstanceManager::unlock_ideology(Ideology const& ideology) {
	std::optional<Date>& spawn_date = ideology_spawn_date[ideology];
	if (!spawn_date.has_value()) {
		spawn_date = instance_manager.get_today();
	} else {
		Logger::warning(
			"Cannot unlock ideology \"", ideology.get_identifier(), "\" - it was already unlocked on: ", *spawn_date
		);
	}
}

void PoliticsInstanceManager::set_great_wars_enabled(bool enabled) {
	// if (enabled && !great_wars_enabled) {
		// TODO - trigger "Great Wars discovered!" popup
	// }

	great_wars_enabled = enabled;
}

void PoliticsInstanceManager::set_world_wars_enabled(bool enabled) {
	if (enabled) {
		set_great_wars_enabled(true);
	}

	world_wars_enabled = enabled;
}
