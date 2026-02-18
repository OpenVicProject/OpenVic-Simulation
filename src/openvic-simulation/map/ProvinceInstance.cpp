#include "ProvinceInstance.hpp"
#include "ProvinceInstanceDeps.hpp"

#include <type_traits>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/MilitaryDefines.hpp"
#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/production/Employee.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/InstanceManager.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/StaticModifierCache.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;

ProvinceInstance::ProvinceInstance(
	ProvinceDefinition const* new_province_definition,
	ProvinceInstanceDeps const* province_instance_deps
) : HasIdentifierAndColour { *new_province_definition },
	HasIndex { new_province_definition->index },
	FlagStrings { "province" },
	PopsAggregate {
		province_instance_deps->stratas,
		province_instance_deps->pop_types,
		province_instance_deps->ideologies
	},
	province_definition { *new_province_definition },
	game_rules_manager { province_instance_deps->game_rules_manager },
	terrain_type { new_province_definition->get_default_terrain_type() },
	rgo { province_instance_deps->rgo_deps },
	pops_cache_by_type { province_instance_deps->pop_types },
	_buildings(
		new_province_definition->is_water()
			? 0
			: province_instance_deps->building_type_manager.get_province_building_types().size(),
			[&province_instance_deps](const size_t i) -> BuildingInstance {
				return *province_instance_deps->building_type_manager.get_province_building_types()[i];
			}
	),
	buildings(_buildings)
{
	modifier_sum.set_this_source(this);
	rgo.setup_location_ptr(*this);
}

ModifierSum const& ProvinceInstance::get_owner_modifier_sum() const {
	return owner->get_modifier_sum();
}

memory::vector<Pop*> const& ProvinceInstance::get_pops_cache_by_type(PopType const& key) const {
	return pops_cache_by_type.at(key);
}

void ProvinceInstance::set_state(State* new_state) {
	// TODO - ensure this is removed from old state and added to new state (either here or wherever this is called from)
	// TODO - update pop factory employment
	state = new_state;
}

GoodDefinition const* ProvinceInstance::get_rgo_good() const {
	if (!rgo.is_valid()) {
		return nullptr;
	}
	return &(rgo.get_production_type_nullable()->output_good);
}

bool ProvinceInstance::set_rgo_production_type_nullable(ProductionType const* rgo_production_type_nullable) {
	bool is_valid_operation = true;
	if (rgo_production_type_nullable != nullptr) {
		ProductionType const& rgo_production_type = *rgo_production_type_nullable;
		if (rgo_production_type.template_type != ProductionType::template_type_t::RGO) {
			spdlog::error_s(
				"Tried setting province {} rgo to {} which is not of template_type RGO.",
				*this, rgo_production_type
			);
			is_valid_operation = false;
		}
		is_valid_operation&=convert_rgo_worker_pops_to_equivalent(rgo_production_type);
	}

	rgo.set_production_type_nullable(rgo_production_type_nullable);
	return is_valid_operation;
}

bool ProvinceInstance::set_owner(CountryInstance* new_owner) {
	bool ret = true;

	if (owner != new_owner) {
		if (owner != nullptr) {
			ret &= owner->remove_owned_province(*this);
		}
		if (new_owner != nullptr) {
			ret &= new_owner->add_owned_province(*this);
		}

		owner = new_owner;

		update_parties_for_votes(new_owner);

		for (Pop& pop : pops) {
			pop.update_location_based_attributes();
		}

		if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::Owner) {
			country_to_report_economy = new_owner;
		} else if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::NeitherWhenOccupied) {
			country_to_report_economy = is_occupied()
				? nullptr
				: owner;
		}
	}

	return ret;
}

bool ProvinceInstance::set_controller(CountryInstance* new_controller) {
	bool ret = true;

	if (new_controller == nullptr) {
		new_controller = owner;
	}

	if (controller != new_controller) {
		if (controller != nullptr) {
			ret &= controller->remove_controlled_province(*this);
		}
		if (new_controller != nullptr) {
			ret &= new_controller->add_controlled_province(*this);
		}

		controller = new_controller;


		if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::Controller) {
			country_to_report_economy = new_controller;
		} else if (game_rules_manager.get_country_to_report_economy() == country_to_report_economy_t::NeitherWhenOccupied) {
			country_to_report_economy = is_occupied()
				? nullptr
				: owner;
		}
	}

	return ret;
}

bool ProvinceInstance::add_core(CountryInstance& new_core, bool warn) {
	if (cores.emplace(&new_core).second) {
		return new_core.add_core_province(*this);
	} else if (warn) {
		spdlog::warn_s(
			"Attempted to add core \"{}\" to province {}: already exists!",
			new_core, *this
		);
	}
	return true;
}

bool ProvinceInstance::remove_core(CountryInstance& core_to_remove, bool warn) {
	if (cores.erase(&core_to_remove) > 0) {
		return core_to_remove.remove_core_province(*this);
	} else if (warn) {
		spdlog::warn_s(
			"Attempted to remove core \"{}\" from province {}: does not exist!",
			core_to_remove, *this
		);
	}
	return true;
}

bool ProvinceInstance::expand_building(
	ModifierEffectCache const& modifier_effect_cache,
	const province_building_index_t index,
	CountryInstance& actor
) {
	return buildings[index].expand(modifier_effect_cache, actor, *this);
}

void ProvinceInstance::_add_pop(Pop&& pop) {
	pop.set_location(*this);
	pops.insert(std::move(pop));
}

bool ProvinceInstance::add_pop_vec(
	std::span<const PopBase> pop_vec,
	PopDeps const& pop_deps
) {
	if (!province_definition.is_water()) {
		reserve_more(pops, pop_vec.size());
		for (PopBase const& pop : pop_vec) {
			_add_pop(Pop {
				pop,
				get_supporter_equivalents_by_ideology().get_keys(),
				pop_deps,
				++last_pop_id
			});
		}
		return true;
	} else {
		spdlog::error_s("Trying to add pop vector to water province {}", *this);
		return false;
	}
}

size_t ProvinceInstance::get_pop_count() const {
	return pops.size();
}

/* REQUIREMENTS:
 * MAP-65, MAP-68, MAP-70, MAP-234
 */
void ProvinceInstance::_update_pops(MilitaryDefines const& military_defines) {
	clear_pops_aggregate();

	has_unaccepted_pops = false;

	for (memory::vector<Pop*>& pops_cache : pops_cache_by_type.get_values()) {
		pops_cache.clear();
	}

	using enum colony_status_t;

	const fixed_point_t pop_size_per_regiment_multiplier =
		colony_status == PROTECTORATE ? military_defines.get_pop_size_per_regiment_protectorate_multiplier()
		: colony_status == COLONY ? military_defines.get_pop_size_per_regiment_colony_multiplier()
		: is_owner_core() ? fixed_point_t::_1 : military_defines.get_pop_size_per_regiment_non_core_multiplier();

	for (Pop& pop : pops) {
		pops_cache_by_type.at(*pop.get_type()).push_back(&pop);
		pop.update_gamestate(military_defines, owner, pop_size_per_regiment_multiplier);
		add_pops_aggregate(pop);
		if (pop.get_culture_status() == Pop::culture_status_t::UNACCEPTED) {
			has_unaccepted_pops = true;
		}
	}

	normalise_pops_aggregate();
}

void ProvinceInstance::update_modifier_sum(Date today, StaticModifierCache const& static_modifier_cache) {
	// Update sum of direct province modifiers
	modifier_sum.clear();

	if (terrain_type != nullptr) {
		modifier_sum.add_modifier(*terrain_type);
	}

	if (province_definition.get_climate() != nullptr) {
		modifier_sum.add_modifier(*province_definition.get_climate());
	}

	if (province_definition.get_continent() != nullptr) {
		modifier_sum.add_modifier(*province_definition.get_continent());
	}

	// Add static modifiers
	if (is_owner_core()) {
		modifier_sum.add_modifier(static_modifier_cache.get_core());
	}
	if (province_definition.is_water()) {
		modifier_sum.add_modifier(static_modifier_cache.get_sea_zone());
	} else {
		modifier_sum.add_modifier(static_modifier_cache.get_land_province());

		modifier_sum.add_modifier(
			province_definition.is_coastal() ? static_modifier_cache.get_coastal() : static_modifier_cache.get_non_coastal()
		);

		// TODO - overseas, blockaded, no_adjacent_controlled, has_siege, occupied, nationalism, infrastructure
	}

	// Erase expired event modifiers and add non-expired ones to the sum
	std::erase_if(event_modifiers, [this, today](ModifierInstance const& modifier) -> bool {
		if (today <= modifier.get_expiry_date()) {
			modifier_sum.add_modifier(*modifier.get_modifier());
			return false;
		} else {
			return true;
		}
	});

	for (BuildingInstance const& building : buildings) {
		modifier_sum.add_modifier(building.building_type);
	}

	if (crime != nullptr) {
		modifier_sum.add_modifier(*crime);
	}

	if (controller != nullptr) {
		controller->contribute_province_modifier_sum(modifier_sum);
	}
}

fixed_point_t ProvinceInstance::get_modifier_effect_value(ModifierEffect const& effect) const {
	using enum ModifierEffect::target_t;

	if (effect.is_local()) {
		// Province-targeted/local effects come from the province itself, only modifiers applied directly to the
		// province contribute to these effects.
		return modifier_sum.get_modifier_effect_value(effect);
	} else if (owner != nullptr) {
		// Non-province targeted/global effects come from the province's owner, even those applied locally
		// (e.g. via a province event modifier) are passed up to the province's controller and only affect the
		// province if the controller is also the owner.
		return owner->get_modifier_effect_value(effect);
	} else {
		return 0;
	}
}

bool ProvinceInstance::convert_rgo_worker_pops_to_equivalent(ProductionType const& production_type) {
	bool is_valid_operation = true;
	std::span<const Job> jobs = production_type.get_jobs();
	for (Pop& pop : pops) {
		for (Job const& job : jobs) {
			PopType const* const job_pop_type = job.get_pop_type();
			PopType const* old_pop_type = pop.get_type();
			if (job_pop_type != old_pop_type) {
				PopType const* const equivalent = old_pop_type->get_equivalent();
				if (job_pop_type == equivalent) {
					is_valid_operation&=pop.convert_to_equivalent();
				}
			}
		}
	}
	return is_valid_operation;
}

void ProvinceInstance::update_gamestate(InstanceManager const& instance_manager) {
	has_empty_adjacent_province = false;
	// We assume there are no duplicate province adjacencies, so each adjacency.get_to() is unique in the loop below
	adjacent_nonempty_land_provinces.clear();

	MapInstance const& map_instance = instance_manager.get_map_instance();
	for (ProvinceDefinition::adjacency_t const& adjacency : province_definition.get_adjacencies()) {
		ProvinceDefinition const& province_definition = *adjacency.get_to();
		ProvinceInstance const& province_instance = map_instance.get_province_instance_by_definition(province_definition);

		if (province_instance.is_empty()) {
			has_empty_adjacent_province = true;
		} else if (!province_definition.is_water()) {
			adjacent_nonempty_land_provinces.push_back(&province_instance);
		}
	}

	land_regiment_count = 0;
	for (ArmyInstance const* army : armies) {
		land_regiment_count += army->get_unit_count();
	}
	for (NavyInstance const* navy : navies) {
		for (ArmyInstance const* army : navy->get_carried_armies()) {
			land_regiment_count += army->get_unit_count();
		}
	}

	if (!is_occupied()) {
		occupation_duration = 0;
	}

	const Date today = instance_manager.get_today();

	for (BuildingInstance& building : buildings) {
		building.update_gamestate(today);
	}
	_update_pops(instance_manager.definition_manager.get_define_manager().get_military_defines());
}

void ProvinceInstance::province_tick(
	const Date today,
	PopValuesFromProvince& reusable_pop_values,
	RandomU32& random_number_generator,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_PROVINCE_TICK
	> reusable_vectors
) {
	if (is_occupied()) {
		++occupation_duration;
	}

	if (!pops.empty()) {
		reusable_pop_values.update_pop_values_from_province(*this);
		for (Pop& pop : pops) {
			pop.pop_tick(
				reusable_pop_values,
				random_number_generator,
				reusable_goods_mask,
				reusable_vectors
			);
		}
	}

	for (BuildingInstance& building : buildings) {
		building.tick(today);
	}
	rgo.rgo_tick(reusable_vectors[0]);
}

bool ProvinceInstance::add_unit_instance_group(UnitInstanceGroup& group) {
	using enum unit_branch_t;

	switch (group.branch) {
	case LAND:
		armies.push_back(static_cast<ArmyInstance*>(&group));
		return true;
	case NAVAL:
		navies.push_back(static_cast<NavyInstance*>(&group));
		return true;
	default:
		spdlog::error_s(
			"Trying to add unit group \"{}\" with invalid branch {} to province {}",
			group.get_name(), static_cast<uint32_t>(group.branch), *this
		);
		return false;
	}
}

bool ProvinceInstance::remove_unit_instance_group(UnitInstanceGroup const& group) {
	const auto remove_from_vector = [this, &group]<unit_branch_t Branch>(
		memory::vector<UnitInstanceGroupBranched<Branch>*>& unit_instance_groups
	) -> bool {
		const typename memory::vector<UnitInstanceGroupBranched<Branch>*>::const_iterator it =
			std::find(unit_instance_groups.begin(), unit_instance_groups.end(), &group);

		if (it != unit_instance_groups.end()) {
			unit_instance_groups.erase(it);
			return true;
		} else {
			spdlog::error_s(
				"Trying to remove non-existent {} \"{}\" from province {}",
				get_branched_unit_group_name(Branch), group.get_name(), *this
			);
			return false;
		}
	};

	using enum unit_branch_t;

	switch (group.branch) {
	case LAND:
		return remove_from_vector(armies);
	case NAVAL:
		return remove_from_vector(navies);
	default:
		spdlog::error_s(
			"Trying to remove unit group \"{}\" with invalid branch {} from province {}",
			group.get_name(), static_cast<uint32_t>(group.branch), *this
		);
		return false;
	}
}

bool ProvinceInstance::apply_history_to_province(ProvinceHistoryEntry const& entry, CountryInstanceManager& country_manager) {
	bool ret = true;

	constexpr auto set_optional = []<typename T>(T& target, std::optional<T> const& source) {
		if (source) {
			target = *source;
		}
	};

	if (entry.get_owner()) {
		ret &= set_owner(&country_manager.get_country_instance_by_definition(**entry.get_owner()));
	}
	if (entry.get_controller()) {
		ret &= set_controller(&country_manager.get_country_instance_by_definition(**entry.get_controller()));
	}
	set_optional(colony_status, entry.get_colonial());
	set_optional(slave, entry.get_slave());
	for (auto const& [country, add] : entry.get_cores()) {
		if (add) {
			ret &= add_core(country_manager.get_country_instance_by_definition(*country), true);
		} else {
			ret &= remove_core(country_manager.get_country_instance_by_definition(*country), true);
		}
	}

	set_optional(life_rating, entry.get_life_rating());
	set_optional(terrain_type, entry.get_terrain_type());
	for (province_building_index_t i(0); i < entry.get_province_building_levels().size(); ++i) {
		const building_level_t level = entry.get_province_building_levels()[i];
		BuildingInstance& building = buildings[i];
		if (level > building_level_t(0) && !building.building_type.can_be_built_in(province_definition)) {
			spdlog::error_s("Building type {} cannot be built in province {}", building.building_type, province_definition);
			ret = false;
		} else {
			building.set_level(level);
		}
	}
	// TODO: load state buildings - entry.get_state_buildings()
	// TODO: party loyalties for each POP when implemented on POP side - entry.get_party_loyalties()
	return ret;
}

void ProvinceInstance::initialise_for_new_game(
	const Date today,
	PopValuesFromProvince& reusable_pop_values,
	RandomU32& random_number_generator,
	IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
	forwardable_span<
		memory::vector<fixed_point_t>,
		VECTORS_FOR_PROVINCE_TICK
	> reusable_vectors
) {
	initialise_rgo();
	province_tick(
		today,
		reusable_pop_values,
		random_number_generator,
		reusable_goods_mask,
		reusable_vectors
	);
}

void ProvinceInstance::initialise_rgo() {
	rgo.initialise_rgo_size_multiplier();
}

void ProvinceInstance::setup_pop_test_values(IssueManager const& issue_manager) {
	for (Pop& pop : pops) {
		pop.setup_pop_test_values(issue_manager);
	}
}

memory::colony<Pop>& ProvinceInstance::get_mutable_pops() {
	return pops;
}

template<typename T>
std::conditional_t<std::is_const_v<T>, Pop const*, Pop*> ProvinceInstance::_find_pop_by_id(T& self, const pop_id_in_province_t pop_id) {
	if (pop_id.is_null()) {
		return nullptr;
	}

	for (std::conditional_t<std::is_const_v<T>, Pop const&, Pop&>& pop : self.pops) {
		if (pop.id_in_province == pop_id) {
			return &pop;
		}
	}

	return nullptr;
}
Pop* ProvinceInstance::find_pop_by_id(const pop_id_in_province_t pop_id) { return _find_pop_by_id(*this, pop_id); }
Pop const* ProvinceInstance::find_pop_by_id(const pop_id_in_province_t pop_id) const { return _find_pop_by_id(*this, pop_id); }