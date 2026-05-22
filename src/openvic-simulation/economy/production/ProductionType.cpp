#include "ProductionType.hpp"

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/population/PopType.hpp"

using namespace OpenVic;

ProductionType::ProductionType(
	GameRulesManager const& new_game_rules_manager,
	const std::string_view new_identifier,
	std::optional<Job>&& new_owner,
	memory::vector<Job>&& new_jobs,
	const template_type_t new_template_type,
	const pop_size_t new_base_workforce_size,
	fixed_point_map_t<GoodDefinition const*>&& new_input_goods,
	GoodDefinition const& new_output_good,
	const fixed_point_t new_base_output_quantity,
	memory::vector<bonus_t>&& new_bonuses,
	fixed_point_map_t<GoodDefinition const*>&& new_maintenance_requirements,
	const bool new_is_coastal,
	const bool new_is_farm,
	const bool new_is_mine
) : HasIdentifier { new_identifier },
	game_rules_manager { new_game_rules_manager },
	owner { std::move(new_owner) },
	jobs { std::move(new_jobs) },
	template_type { new_template_type },
	base_workforce_size { new_base_workforce_size },
	input_goods { std::move(new_input_goods) },
	output_good { new_output_good },
	base_output_quantity { new_base_output_quantity },
	bonuses { std::move(new_bonuses) },
	maintenance_requirements { std::move(new_maintenance_requirements) },
	is_coastal { new_is_coastal },
	_is_farm { new_is_farm },
	_is_mine { new_is_mine } {}


bool ProductionType::get_is_farm_for_tech() const {
	if (game_rules_manager.get_use_simple_farm_mine_logic()) {
		return _is_farm;
	}

	return !_is_mine && _is_farm;
}
bool ProductionType::get_is_mine_for_non_tech() const {
	if (game_rules_manager.get_use_simple_farm_mine_logic()) {
		return _is_mine;
	}

	return !_is_farm;
}

bool ProductionType::is_valid_for_artisan_in(ProvinceInstance& province) const{
	if (template_type != template_type_t::ARTISAN) {
		return false;
	}
	return !is_coastal || game_rules_manager.may_use_coastal_artisanal_production_types(province);
}

bool ProductionType::is_valid_for_factory_in(State& state) const{
	if (template_type != template_type_t::FACTORY) {
		return false;
	}
	return !is_coastal || game_rules_manager.may_use_coastal_factory_production_types(state);
}

bool ProductionType::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (auto& [bonus_script, bonus_value] : bonuses) {
		ret &= bonus_script.parse_script(false, definition_manager);
	}
	return ret;
}
