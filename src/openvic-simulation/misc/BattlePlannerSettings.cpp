#include "BattlePlannerSettings.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Tool::Option::Option(std::string_view new_name) : name { new_name } {}

Tool::Tool(
	std::string_view new_identifier, std::string_view new_icon_identifier, std::vector<Tool::Option> new_options
) : HasIdentifier { new_identifier }, icon_identifier { new_icon_identifier }, options { new_options } {}

/*bool Tool::_set_options(std::vector<Tool::Option> new_options){
	options = new_options;
}*/

bool BattlePlannerSettingsManager::_load_tool(std::string_view tool_identifier, ast::NodeCPtr root){
	std::string icon_identifier {};
	std::vector<Tool::Option> options {};

	bool ret = expect_dictionary_keys(
		"icon", ONE_EXACTLY, expect_identifier(assign_variable_callback_string(icon_identifier)),
		"option", ONE_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			std::string_view option_name {}, type_name {};
			fixed_point_t red = 0, green = 0, blue = 0;
			if (!expect_dictionary_keys<StringMapCaseInsensitive>(
				"name", ONE_EXACTLY, expect_identifier(assign_variable_callback(option_name)),
				"type", ZERO_OR_ONE, expect_identifier(assign_variable_callback(type_name)),
				"red", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(red)),
				"green", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(green)),
				"blue", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(blue))
			)(node)) {
				Logger::error("Failed to load Option for BattlePlanner Tool");
				return false;
			}
			colour_t colour = colour_t::from_floats(red, green, blue);
			Tool::Option option = { option_name };
			option.colour = colour;
			option.type = type_name;
			
			options.push_back(option);
			return true;
		}
	)(root);

	if (tool_identifier.empty()) {
		Logger::error("Invalid Battle Planner Tool identifier - empty!");
		return false;
	}
	if (icon_identifier.empty()) {
		Logger::error("Invalid Battle Planner Tool Icon identifier - empty!");
		return false;
	}

	Tool tool = {tool_identifier, icon_identifier};

	ret &= tools.add_item(tool);
	return ret;
}

bool BattlePlannerSettingsManager::load_battleplannersettings_file(ast::NodeCPtr root){
	return expect_dictionary_reserve_length(tools,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			return _load_tool(key, value);
		}
	)(root);
}

/*
Actor::Attachment::Attachment(std::string_view new_actor_name, std::string_view new_attach_node, attach_id_t new_attach_id)
  : actor_name { new_actor_name }, attach_node { new_attach_node }, attach_id { new_attach_id } {}

Actor::Animation::Animation(std::string_view new_file, fixed_point_t new_scroll_time)
  : file { new_file }, scroll_time { new_scroll_time } {}

Actor::Actor() : model_file {}, scale { 1 }, idle_animation {}, move_animation {}, attack_animation {} {}


bool Actor::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"actorfile", ONE_EXACTLY, expect_string(assign_variable_callback_string(model_file)),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),

		"attach", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			std::string_view actor_name {}, attach_node {};
			Attachment::attach_id_t attach_id = 0;

			if (!expect_dictionary_keys<StringMapCaseInsensitive>(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(actor_name)),
				"node", ONE_EXACTLY, expect_string(assign_variable_callback(attach_node)),
				"attachId", ONE_EXACTLY, expect_uint(assign_variable_callback(attach_id))
			)(node)) {
				Logger::error("Failed to load attachment for actor ", get_name());
				return false;
			}

			attachments.push_back({ actor_name, attach_node, attach_id });
			return true;
		},

		"idle", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "idle", std::placeholders::_1, 0)),
		"move", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "move", std::placeholders::_1, 0)),
		"attack", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "attack", std::placeholders::_1, 0)),
		"animation", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			std::string_view name {}, file {};
			fixed_point_t scroll_time = 0;

			if (!expect_dictionary_keys<StringMapCaseInsensitive>(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
				"file", ONE_EXACTLY, expect_string(assign_variable_callback(file)),
				"defaultAnimationTime", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scroll_time))
			)(node)) {
				Logger::error("Failed to load animation for actor ", get_name());
				return false;
			}

			return _set_animation(name, file, scroll_time);
		}
	);

	return ret;
}
*/