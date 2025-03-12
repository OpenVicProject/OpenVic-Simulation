#include "Deployment.hpp"

#include "openvic-simulation/DefinitionManager.hpp" /* gosh don't we all just love circular inclusion :DDD */

using namespace OpenVic;
using namespace OpenVic::NodeTools;

UnitDeployment<UnitType::branch_t::LAND>::UnitDeployment(
	std::string_view new_name, RegimentType const& new_type, ProvinceDefinition const* new_home
) : name { new_name }, type { new_type }, home { new_home } {}

UnitDeployment<UnitType::branch_t::NAVAL>::UnitDeployment(std::string_view new_name, ShipType const& new_type)
	: name { new_name }, type { new_type } {}

template<UnitType::branch_t Branch>
UnitDeploymentGroup<Branch>::UnitDeploymentGroup(
	std::string_view new_name, ProvinceDefinition const* new_location, std::vector<_Unit>&& new_units,
	std::optional<size_t> new_leader_index
) : name { new_name }, location { new_location }, units { std::move(new_units) }, leader_index { new_leader_index } {}

Deployment::Deployment(
	std::string_view new_path, std::vector<ArmyDeployment>&& new_armies, std::vector<NavyDeployment>&& new_navies,
	std::vector<LeaderBase>&& new_leaders
) : HasIdentifier { new_path }, armies { std::move(new_armies) }, navies { std::move(new_navies) },
	leaders { std::move(new_leaders) } {}

bool DeploymentManager::add_deployment(
	std::string_view path, std::vector<ArmyDeployment>&& armies, std::vector<NavyDeployment>&& navies,
	std::vector<LeaderBase>&& leaders
) {
	if (path.empty()) {
		Logger::error("Attempted to load order of battle with no path! Something is very wrong!");
		return false;
	}

	return deployments.add_item({ path, std::move(armies), std::move(navies), std::move(leaders) });
}

bool DeploymentManager::load_oob_file(
	DefinitionManager const& definition_manager, Dataloader const& dataloader, std::string_view history_path,
	Deployment const*& deployment, bool fail_on_missing
) {
	deployment = get_deployment_by_identifier(history_path);
	if (deployment != nullptr) {
		Logger::warning("Loading an already-loaded OOB file with path ", history_path);
		return true;
	}

	if (missing_oob_files.contains(history_path)) {
		return !fail_on_missing;
	}

	static constexpr std::string_view oob_directory = "history/units/";

	const fs::path lookedup_path =
		dataloader.lookup_file(StringUtils::append_string_views(oob_directory, history_path), false);

	if (lookedup_path.empty()) {
		missing_oob_files.emplace(history_path);
		if (fail_on_missing) {
			Logger::warning("Could not find OOB file ", history_path, "!");
			return false;
		} else {
			return true;
		}
	}

	std::vector<ArmyDeployment> armies;
	std::vector<NavyDeployment> navies;
	std::vector<LeaderBase> leaders;

	size_t general_count = 0, admiral_count = 0;

	using enum UnitType::branch_t;

	const auto leader_callback = [&leaders, &definition_manager, &general_count, &admiral_count](ast::NodeCPtr node) -> bool {
		std::string_view leader_name {};
		UnitType::branch_t leader_branch = INVALID_BRANCH;
		Date leader_date {};
		LeaderTrait const* leader_personality = nullptr;
		LeaderTrait const* leader_background = nullptr;
		fixed_point_t leader_prestige = 0;
		std::string_view picture {};

		bool ret = expect_dictionary_keys(
			"name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(leader_name)),
			"date", ONE_EXACTLY, expect_date_identifier_or_string(assign_variable_callback(leader_date)),
			"type", ONE_EXACTLY, UnitTypeManager::expect_branch_identifier(assign_variable_callback(leader_branch)),
			"personality", ONE_EXACTLY, definition_manager.get_military_manager().get_leader_trait_manager()
				.expect_leader_trait_identifier_or_string(assign_variable_callback_pointer(leader_personality)),
			"background", ONE_EXACTLY, definition_manager.get_military_manager().get_leader_trait_manager()
				.expect_leader_trait_identifier_or_string(assign_variable_callback_pointer(leader_background)),
			"prestige", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(leader_prestige)),
			"picture", ZERO_OR_ONE, expect_identifier_or_string(assign_variable_callback(picture))
		)(node);

		if (leader_name.empty()) {
			Logger::error("Leader has a missing or empty name!");
			ret = false;
		}

		if (leader_personality != nullptr && !leader_personality->is_personality_trait()) {
			Logger::error(
				"Leader ", leader_name, " has personality ", leader_personality->get_identifier(),
				" which is not a personality trait!"
			);
			ret = false;
		}

		if (leader_background != nullptr && !leader_background->is_background_trait()) {
			Logger::error(
				"Leader ", leader_name, " has background ", leader_background->get_identifier(),
				" which is not a background trait!"
			);
			ret = false;
		}

		switch (leader_branch) {
		case LAND:
			++general_count;
			break;
		case NAVAL:
			++admiral_count;
			break;
		default:
			Logger::error("Invalid branch ", static_cast<uint64_t>(leader_branch), " for leader ", leader_name);
			return false;
		}

		leaders.push_back(
			{ leader_name, leader_branch, leader_date, leader_personality, leader_background, leader_prestige, picture }
		);

		return ret;
	};

	bool ret = expect_dictionary_keys_and_default(
		key_value_success_callback, // TODO: load SOI information
		"leader", ZERO_OR_MORE, leader_callback,
		"army", ZERO_OR_MORE, [&armies, &general_count, &definition_manager, &leader_callback](ast::NodeCPtr node) -> bool {
			std::string_view army_name {};
			ProvinceDefinition const* army_location = nullptr;
			std::vector<RegimentDeployment> army_regiments {};

			const size_t starting_general_count = general_count;

			const bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(army_name)),
				"location", ONE_EXACTLY, definition_manager.get_map_definition().expect_province_definition_identifier(
					assign_variable_callback_pointer(army_location)
				),
				"regiment", ONE_OR_MORE, [&definition_manager, &army_regiments](ast::NodeCPtr node) -> bool {
					std::string_view regiment_name {};
					RegimentType const* regiment_type = nullptr;
					ProvinceDefinition const* regiment_home = nullptr;

					const bool ret = expect_dictionary_keys(
						"name", ONE_EXACTLY, expect_string(assign_variable_callback(regiment_name)),
						"type", ONE_EXACTLY, definition_manager.get_military_manager().get_unit_type_manager()
							.expect_regiment_type_identifier(assign_variable_callback_pointer(regiment_type)),
						"home", ZERO_OR_ONE, definition_manager.get_map_definition()
							.expect_province_definition_identifier(assign_variable_callback_pointer(regiment_home))
					)(node);

					if (regiment_home == nullptr) {
						Logger::warning("RegimentDeployment ", regiment_name, " has no home province!");
					}

					if (regiment_type == nullptr) {
						Logger::error("RegimentDeployment ", regiment_name, " has no type!");
						return false;
					}

					army_regiments.push_back({ regiment_name, *regiment_type, regiment_home });

					return ret;
				},
				"leader", ZERO_OR_ONE, leader_callback
			)(node);

			armies.push_back(
				{
					army_name, army_location, std::move(army_regiments),
					starting_general_count < general_count ? std::optional { general_count - 1 } : std::nullopt
				}
			);

			return ret;
		},
		"navy", ZERO_OR_MORE, [&navies, &admiral_count, &definition_manager, &leader_callback](ast::NodeCPtr node) -> bool {
			std::string_view navy_name {};
			ProvinceDefinition const* navy_location = nullptr;
			std::vector<ShipDeployment> navy_ships {};

			const size_t starting_admiral_count = admiral_count;

			const bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(navy_name)),
				"location", ONE_EXACTLY, definition_manager.get_map_definition().expect_province_definition_identifier(
					assign_variable_callback_pointer(navy_location)
				),
				"ship", ONE_OR_MORE, [&definition_manager, &navy_ships](ast::NodeCPtr node) -> bool {
					std::string_view ship_name {};
					ShipType const* ship_type = nullptr;

					const bool ret = expect_dictionary_keys(
						"name", ONE_EXACTLY, expect_string(assign_variable_callback(ship_name)),
						"type", ONE_EXACTLY, definition_manager.get_military_manager().get_unit_type_manager()
							.expect_ship_type_identifier(assign_variable_callback_pointer(ship_type))
					)(node);

					if (ship_type == nullptr) {
						Logger::error("ShipDeployment ", ship_name, " has no type!");
						return false;
					}

					navy_ships.push_back({ ship_name, *ship_type });

					return ret;
				},
				"leader", ZERO_OR_ONE, leader_callback
			)(node);

			navies.push_back(
				{
					navy_name, navy_location, std::move(navy_ships),
					starting_admiral_count < admiral_count ? std::optional { admiral_count - 1 } : std::nullopt
				}
			);

			return ret;
		}
	)(Dataloader::parse_defines(lookedup_path).get_file_node());

	if (general_count + admiral_count != leaders.size()) {
		Logger::error(
			"Mismatch in sum (#", general_count + admiral_count, ") of general (#", general_count, ") and admiral (#",
			admiral_count, ") counts when compared to loaded leader count (#", leaders.size(), ") for OOB file ",
			history_path
		);
		return false;
	}

	if (add_deployment(history_path, std::move(armies), std::move(navies), std::move(leaders))) {
		deployment = &get_back_deployment();
	} else {
		ret = false;
	}

	return ret;
}

size_t DeploymentManager::get_missing_oob_file_count() const {
	return missing_oob_files.size();
}
