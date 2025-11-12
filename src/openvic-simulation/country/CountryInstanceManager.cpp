#include "CountryInstanceManager.hpp"
#include <algorithm>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/history/CountryHistory.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

using namespace OpenVic;

CountryInstanceManager::CountryInstanceManager(
	CountryDefines const& new_country_defines,
	CountryDefinitionManager const& new_country_definition_manager,
	CountryInstanceDeps const& country_instance_deps,
	GoodInstanceManager const& new_good_instance_manager,
	PopsDefines const& new_pop_defines,
	utility::forwardable_span<const PopType> pop_type_keys,
	ThreadPool& new_thread_pool
) : thread_pool { new_thread_pool },
  	country_definition_manager { new_country_definition_manager },
	country_defines { new_country_defines },
	shared_country_values {
		new_pop_defines,
		new_good_instance_manager,
		pop_type_keys
	},
	country_instance_by_definition {
		new_country_definition_manager.get_country_definitions(),
		[
			this,
			country_instance_deps_ptr=&country_instance_deps
		](CountryDefinition const& country_definition)->auto{
			return std::make_tuple(
				&country_definition,
				&shared_country_values,
				country_instance_deps_ptr
			);
		}
	}
{
	assert(new_country_definition_manager.country_definitions_are_locked());
	great_powers.reserve(16);
	secondary_powers.reserve(16);
}

void CountryInstanceManager::update_rankings(const Date today) {
	total_ranking.clear();

	for (CountryInstance& country : get_country_instances()) {
		if (country.exists()) {
			total_ranking.push_back(&country);
		}
	}

	prestige_ranking = total_ranking;
	industrial_power_ranking = total_ranking;
	military_power_ranking = total_ranking;

	std::stable_sort(
		total_ranking.begin(), total_ranking.end(),
		[](CountryInstance* a, CountryInstance* b) -> bool {
			const bool a_civilised = a->is_civilised();
			const bool b_civilised = b->is_civilised();
			return a_civilised != b_civilised ? a_civilised : a->total_score.get_untracked() > b->total_score.get_untracked();
		}
	);
	std::stable_sort(
		prestige_ranking.begin(), prestige_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_prestige_untracked() > b->get_prestige_untracked();
		}
	);
	std::stable_sort(
		industrial_power_ranking.begin(), industrial_power_ranking.end(),
		[](CountryInstance const* a, CountryInstance const* b) -> bool {
			return a->get_industrial_power_untracked() > b->get_industrial_power_untracked();
		}
	);
	std::stable_sort(
		military_power_ranking.begin(), military_power_ranking.end(),
		[](CountryInstance* a, CountryInstance* b) -> bool {
			return a->military_power.get_untracked() > b->military_power.get_untracked();
		}
	);

	for (size_t index = 0; index < total_ranking.size(); ++index) {
		const size_t rank = index + 1;
		total_ranking[index]->total_rank = rank;
		prestige_ranking[index]->prestige_rank = rank;
		industrial_power_ranking[index]->industrial_rank = rank;
		military_power_ranking[index]->military_rank = rank;
	}

	const size_t max_great_power_rank = country_defines.get_great_power_rank();
	const size_t max_secondary_power_rank = country_defines.get_secondary_power_rank();
	const Timespan lose_great_power_grace_days = country_defines.get_lose_great_power_grace_days();

	using enum CountryInstance::country_status_t;
	// Demote great powers who have been below the max great power rank for longer than the demotion grace period and
	// remove them from the list. We don't just demote them all and clear the list as when rebuilding we'd need to look
	// ahead for countries below the max great power rank but still within the demotion grace period.
	std::erase_if(great_powers, [max_great_power_rank, today](CountryInstance* great_power) -> bool {
		if (OV_likely(great_power->get_country_status() == COUNTRY_STATUS_GREAT_POWER)) {
			if (OV_unlikely(
				great_power->get_total_rank() > max_great_power_rank && great_power->get_lose_great_power_date() < today
			)) {
				great_power->country_status = COUNTRY_STATUS_CIVILISED;
				return true;
			} else {
				return false;
			}
		}
		return true;
	});

	// Demote all secondary powers and clear the list. We will rebuilt the whole list from scratch, so there's no need to
	// keep countries which are still above the max secondary power rank (they might become great powers instead anyway).
	for (CountryInstance* secondary_power : secondary_powers) {
		if (secondary_power->country_status == COUNTRY_STATUS_SECONDARY_POWER) {
			secondary_power->country_status = COUNTRY_STATUS_CIVILISED;
		}
	}
	secondary_powers.clear();

	// Calculate the maximum number of countries eligible for great or secondary power status. This accounts for the
	// possibility of the max secondary power rank being higher than the max great power rank or both being zero, just
	// in case someone wants to experiment with only having secondary powers when some great power slots are filled by
	// countries in the demotion grace period, or having no great or secondary powers at all.
	const size_t max_power_index = std::clamp(max_secondary_power_rank, max_great_power_rank, total_ranking.size());

	for (size_t index = 0; index < max_power_index; index++) {
		CountryInstance* country = total_ranking[index];

		if (!country->is_civilised()) {
			// All further countries are civilised and so ineligible for great or secondary power status.
			break;
		}

		if (country->is_great_power()) {
			// The country already has great power status and is in the great powers list.
			continue;
		}

		if (great_powers.size() < max_great_power_rank && country->get_total_rank() <= max_great_power_rank) {
			// The country is eligible for great power status and there are still slots available,
			// so it is promoted and added to the list.
			country->country_status = COUNTRY_STATUS_GREAT_POWER;
			great_powers.push_back(country);
		} else if (country->get_total_rank() <= max_secondary_power_rank) {
			// The country is eligible for secondary power status and so is promoted and added to the list.
			country->country_status = COUNTRY_STATUS_SECONDARY_POWER;
			secondary_powers.push_back(country);
		}
	}

	// Sort the great powers list by total rank, as pre-existing great powers may have changed rank order and new great
	// powers will have been added to the end of the list regardless of rank.
	std::stable_sort(great_powers.begin(), great_powers.end(), [](CountryInstance const* a, CountryInstance const* b) -> bool {
		return a->get_total_rank() < b->get_total_rank();
	});

	// Update the lose great power date for all great powers which are above the max great power rank.
	const Date new_lose_great_power_date = today + lose_great_power_grace_days;
	for (CountryInstance* great_power : great_powers) {
		if (great_power->get_total_rank() <= max_great_power_rank) {
			great_power->lose_great_power_date = new_lose_great_power_date;
		}
	}
}

CountryInstance* CountryInstanceManager::get_country_instance_by_identifier(std::string_view identifier) {
	CountryDefinition const* country_definition = country_definition_manager.get_country_definition_by_identifier(identifier);
	return country_definition == nullptr
		? nullptr
		: &get_country_instance_by_definition(*country_definition);
}
CountryInstance const* CountryInstanceManager::get_country_instance_by_identifier(std::string_view identifier) const {
	CountryDefinition const* country_definition = country_definition_manager.get_country_definition_by_identifier(identifier);
	return country_definition == nullptr
		? nullptr
		: &get_country_instance_by_definition(*country_definition);
}
CountryInstance* CountryInstanceManager::get_country_instance_by_index(typename CountryInstance::index_t index) {
	return country_instance_by_definition.contains_index(index)
		? &country_instance_by_definition.at_index(index)
		: nullptr;
}
CountryInstance const* CountryInstanceManager::get_country_instance_by_index(typename CountryInstance::index_t index) const {
	return country_instance_by_definition.contains_index(index)
		? &country_instance_by_definition.at_index(index)
		: nullptr;
}
CountryInstance& CountryInstanceManager::get_country_instance_by_definition(CountryDefinition const& country_definition) {
	return country_instance_by_definition.at(country_definition);
}
CountryInstance const& CountryInstanceManager::get_country_instance_by_definition(CountryDefinition const& country_definition) const {
	return country_instance_by_definition.at(country_definition);
}

bool CountryInstanceManager::apply_history_to_countries(InstanceManager& instance_manager) {
	CountryHistoryManager const& history_manager =
		instance_manager.get_definition_manager().get_history_manager().get_country_manager();

	bool ret = true;

	const Date today = instance_manager.get_today();
	UnitInstanceManager& unit_instance_manager = instance_manager.get_unit_instance_manager();
	MapInstance& map_instance = instance_manager.get_map_instance();

	const Date starting_last_war_loss_date = today - RECENT_WAR_LOSS_TIME_LIMIT;
	FlagStrings& global_flags = instance_manager.get_global_flags();

	for (CountryInstance& country_instance : get_country_instances()) {
		country_instance.last_war_loss_date = starting_last_war_loss_date;

		if (!country_instance.get_country_definition().is_dynamic_tag()) {
			CountryHistoryMap const* history_map = history_manager.get_country_history(
				country_instance.get_country_definition()
			);

			if (history_map != nullptr) {
				static constexpr fixed_point_t DEFAULT_STATE_CULTURE_LITERACY = fixed_point_t::_0_50;
				CountryHistoryEntry const* oob_history_entry = nullptr;
				std::optional<fixed_point_t> state_culture_consciousness;
				std::optional<fixed_point_t> nonstate_culture_consciousness;
				fixed_point_t state_culture_literacy = DEFAULT_STATE_CULTURE_LITERACY;
				std::optional<fixed_point_t> nonstate_culture_literacy;

				for (auto const& [entry_date, entry] : history_map->get_entries()) {
					if (entry_date <= today) {
						ret &= country_instance.apply_history_to_country(
							*entry,
							*this,
							global_flags,
							map_instance
						);

						if (entry->get_initial_oob().has_value()) {
							oob_history_entry = entry.get();
						}
						if (entry->get_consciousness().has_value()) {
							state_culture_consciousness = entry->get_consciousness();
						}
						if (entry->get_nonstate_consciousness().has_value()) {
							nonstate_culture_consciousness = entry->get_nonstate_consciousness();
						}
						if (entry->get_literacy().has_value()) {
							state_culture_literacy = *entry->get_literacy();
						}
						if (entry->get_nonstate_culture_literacy().has_value()) {
							nonstate_culture_literacy = entry->get_nonstate_culture_literacy();
						}
					} else {
						// All foreign investments are applied regardless of the bookmark's date
						country_instance.apply_foreign_investments(entry->get_foreign_investment(), *this);
					}
				}

				if (oob_history_entry != nullptr) {
					ret &= unit_instance_manager.generate_deployment(
						map_instance, country_instance, *oob_history_entry->get_initial_oob()
					);
				}

				// TODO - check if better to do "if"s then "for"s, so looping multiple times rather than having lots of
				// redundant "if" statements?
				for (ProvinceInstance* province : country_instance.get_owned_provinces()) {
					for (Pop& pop : province->get_mutable_pops()) {
						if (country_instance.is_primary_or_accepted_culture(pop.get_culture())) {
							pop.set_literacy(state_culture_literacy);

							if (state_culture_consciousness.has_value()) {
								pop.set_consciousness(*state_culture_consciousness);
							}
						} else {
							if (nonstate_culture_literacy.has_value()) {
								pop.set_literacy(*nonstate_culture_literacy);
							}

							if (nonstate_culture_consciousness.has_value()) {
								pop.set_consciousness(*nonstate_culture_consciousness);
							}
						}
					}
				}
			} else {
				spdlog::error_s("Country {} has no history!", country_instance);
				ret = false;
			}
		}
	}
	shared_country_values.update_costs();
	return ret;
}

void CountryInstanceManager::update_modifier_sums(const Date today, StaticModifierCache const& static_modifier_cache) {
	for (CountryInstance& country : get_country_instances()) {
		country.update_modifier_sum(today, static_modifier_cache);
	}
}

void CountryInstanceManager::update_gamestate(const Date today, MapInstance& map_instance) {
	for (CountryInstance& country : get_country_instances()) {
		country.update_gamestate(today, map_instance);
	}

	// TODO - work out how to have ranking effects applied (e.g. static modifiers) applied at game start
	// we can't just move update_rankings to the top of this function as it will choose initial GPs based on
	// incomplete scores. Although we should check if the base game includes all info or if it really does choose
	// starting GPs based purely on stuff like prestige which is set by history before the first game update.
	update_rankings(today);
}

void CountryInstanceManager::country_manager_tick_before_map() {
	thread_pool.process_country_ticks_before_map();
}

void CountryInstanceManager::country_manager_tick_after_map() {
	thread_pool.process_country_ticks_after_map();
	shared_country_values.update_costs();
}
