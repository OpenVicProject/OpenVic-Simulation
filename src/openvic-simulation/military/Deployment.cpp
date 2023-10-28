#include "Deployment.hpp"

#include "openvic-simulation/GameManager.hpp" /* gosh don't we all just love circular inclusion :DDD */

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Deployment::Deployment(
	std::string_view new_path, std::vector<Army>&& new_armies,
	std::vector<Navy>&& new_navies, std::vector<Leader>&& new_leaders
) : HasIdentifier { new_path }, armies { std::move(new_armies) },
	navies { std::move(new_navies) }, leaders { std::move(new_leaders) } {}

const std::vector<Army>& Deployment::get_armies() const {
	return armies;
}

const std::vector<Navy>& Deployment::get_navies() const {
	return navies;
}

const std::vector<Leader>& Deployment::get_leaders() const {
	return leaders;
}

DeploymentManager::DeploymentManager() : deployments { "deployments" } {}

bool DeploymentManager::add_deployment(std::string_view path, std::vector<Army>&& armies,
	std::vector<Navy>&& navies, std::vector<Leader>&& leaders) {
	if (path.empty()) {
		Logger::error("Attemped to load order of battle with no path! Something is very wrong!");
		return false;
	}
	if (armies.empty() && navies.empty() && leaders.empty() && path != "NULL") {
		Logger::warning("Loading redundant empty order of battle at ", path);
	}

	return deployments.add_item({ path, std::move(armies), std::move(navies), std::move(leaders) });
}

bool DeploymentManager::load_oob_file(GameManager& game_manager, std::string_view path, ast::NodeCPtr root) {
	std::vector<Army> armies;
	std::vector<Navy> navies;
	std::vector<Leader> leaders;

	bool ret = expect_dictionary_keys_and_default(
		key_value_success_callback, // TODO: load SOI information
		"leader", ZERO_OR_MORE, [&leaders, &game_manager](ast::NodeCPtr node) -> bool {
			std::string_view name;
			Unit::type_t type;
			Date date;
			LeaderTrait const* personality = nullptr;
			LeaderTrait const* background = nullptr;
			fixed_point_t prestige = 0;

			bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name), false),
				"date", ONE_EXACTLY, expect_identifier_or_string(expect_date_str(assign_variable_callback(date))),
				"type", ONE_EXACTLY, expect_identifier([&type](std::string_view leader_type) -> bool {
					if (leader_type == "land") {
						type = Unit::type_t::LAND;
					} else {
						type = Unit::type_t::NAVAL;
					}
					return true;
				}),
				"personality", ONE_EXACTLY, game_manager.get_military_manager().get_leader_trait_manager()
					.expect_leader_trait_identifier(assign_variable_callback_pointer(personality)),
				"background", ONE_EXACTLY, game_manager.get_military_manager().get_leader_trait_manager()
					.expect_leader_trait_identifier(assign_variable_callback_pointer(background)),
				"prestige", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(prestige)),
				"picture", ZERO_OR_ONE, success_callback
			)(node);

			if (!personality->is_personality_trait()) {
				Logger::error("Leader ", name, " has personality ", personality->get_identifier(),
					" which is not a personality trait!");
				return false;
			}
			if (!background->is_background_trait()) {
				Logger::error("Leader ", name, " has background ", background->get_identifier(),
					" which is not a background trait!");
				return false;
			}

			leaders.push_back(Leader{ std::string(name), type, date, personality, background, prestige });
			return ret;
		},
		"army", ZERO_OR_MORE, [&armies, &game_manager](ast::NodeCPtr node) -> bool {
			std::string_view name;
			Province const* location = nullptr;
			std::vector<Regiment> regiments;

			bool ret = expect_dictionary_keys(
				/* another paradox gem, tested in game and they don't lead the army or even show up */
				"leader", ZERO_OR_MORE, success_callback,
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name), false),
				"location", ONE_EXACTLY,
					game_manager.get_map().expect_province_identifier(assign_variable_callback_pointer(location)),
				"regiment", ONE_OR_MORE, [&game_manager, &regiments](ast::NodeCPtr node) -> bool {
					Regiment regiment;
					bool ret = expect_dictionary_keys(
						"name", ONE_EXACTLY, expect_string(assign_variable_callback_string(regiment.name), false),
						"type", ONE_EXACTLY, game_manager.get_military_manager().get_unit_manager()
							.expect_unit_identifier(assign_variable_callback_pointer(regiment.type)),
						"home", ONE_EXACTLY, game_manager.get_map()
							.expect_province_identifier(assign_variable_callback_pointer(regiment.home))
					)(node);
					regiments.push_back(regiment);
					return ret;
				}
			)(node);
			armies.push_back(Army{ std::string(name), location, std::move(regiments) });
			return ret;
		},
		"navy", ZERO_OR_MORE, [&navies, &game_manager](ast::NodeCPtr node) -> bool {
			std::string_view name;
			Province const* location = nullptr;
			std::vector<Ship> ships;

			bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name), false),
				"location", ONE_EXACTLY,
					game_manager.get_map().expect_province_identifier(assign_variable_callback_pointer(location)),
				"ship", ONE_OR_MORE, [&game_manager, &ships](ast::NodeCPtr node) -> bool {
					Ship ship;
					bool ret = expect_dictionary_keys(
						"name", ONE_EXACTLY, expect_string(assign_variable_callback_string(ship.name), false),
						"type", ONE_EXACTLY, game_manager.get_military_manager().get_unit_manager()
							.expect_unit_identifier(assign_variable_callback_pointer(ship.type))
					)(node);
					ships.push_back(ship);
					return ret;
				},
				"leader", ZERO_OR_MORE, success_callback
			)(node);
			navies.push_back(Navy{ std::string(name), location, std::move(ships) });
			return ret;
		}
	)(root);
	/* need to do this for platform compatibility of identifiers */
	std::string identifier = std::string { path };
	std::replace(identifier.begin(), identifier.end(), '\\', '/');
	ret &= add_deployment(identifier, std::move(armies), std::move(navies), std::move(leaders));

	return ret;
}
