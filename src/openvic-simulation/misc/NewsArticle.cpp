#include "NewsArticle.hpp"

#include <openvic-dataloader/v2script/Parser.hpp>
#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

//nope: not all news things are typed, so we want a more general function
node_callback_t Typed::expect_typed_objects(length_callback_t length_callback, callback_t<std::unique_ptr<Typed>&&> callback) {
	return expect_dictionary_keys_and_length(
		length_callback,
		//_expect_instance<Typed, Actor>(callback)
		/*"news_priority", ZERO_OR_MORE, expect_dictionary_keys(
			"case", ZERO_OR_MORE, success_callback
		),*/
		"news_priority", ZERO_OR_MORE, _expect_instance<Typed, NewsPriority>(callback),
		"generator_selector", ZERO_OR_MORE, expect_dictionary_keys(
			"case", ZERO_OR_MORE, success_callback,
			"@2@", ZERO_OR_MORE, success_callback//expect_identifier_or_string()
		),
		"generate_article", ZERO_OR_MORE, expect_dictionary_keys(
			"size", ZERO_OR_MORE, success_callback,
			"picture_case", ZERO_OR_MORE, success_callback,
			"title_case", ZERO_OR_MORE, success_callback,
			"description_case", ZERO_OR_MORE, success_callback
		),
		"on_printing", ZERO_OR_MORE, expect_dictionary_keys(
			"effect", ZERO_OR_MORE, success_callback
		)//,
		/*"on_collection", ZERO_OR_ONE, expect_dictionary_keys(
			"effect", ZERO_OR_MORE, success_callback
		)*/
	);

}



/*
		Case(size_t new_index, fixed_point_t new_value, fixed_point_t priority_add,
		 std::string_view picture_path, ConditionScript&& new_trigger);
*/

Case::Case(fixed_point_t new_value, fixed_point_t new_priority_add,
 std::string_view picture_path, ConditionScript&& new_trigger) :
 value { 0 }, priority_add { 0 }, picture { }, trigger { std::move(new_trigger) } {}
 /*
 size_t new_index, ...
 : HasIdentifier { std::to_string(new_index) }, value { 0 }, priority_add { 0 },
  picture { }, trigger { std::move(new_trigger) } {}*/

bool Case::parse_scripts(DefinitionManager const& definition_manager) {
	return trigger.parse_script(true, definition_manager);
}

bool Case::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	return add_key_map_entries(key_map,
		"trigger", ZERO_OR_ONE, trigger.expect_script(),
		"value", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(value)),
		"picture", ZERO_OR_ONE, expect_string(assign_variable_callback_string(picture)),
		"priority_add", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(priority_add))
	);
}

//NewsPriority::NewsPriority() : priority_case { } {}

bool NewsPriority::_fill_key_map(NodeTools::case_insensitive_key_map_t &key_map){
	return true;
}




/*
bool UIManager::load_gfx_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"spriteTypes", ZERO_OR_ONE, Sprite::expect_sprites(
			NodeTools::reserve_length_callback(sprites),
			[this](std::unique_ptr<Sprite>&& sprite) -> bool {
				return sprites.add_item(std::move(sprite), duplicate_warning_callback);
			}
		),
		"fonts", ZERO_OR_ONE, _load_fonts("font"),
		"objectTypes", ZERO_OR_ONE, Object::expect_objects(
			NodeTools::reserve_length_callback(objects),
			[this](std::unique_ptr<Object>&& object) -> bool {
				return objects.add_item(std::move(object), duplicate_warning_callback);
			}
		)
	)(root);
}

*/


/*
				EffectScript effect;
				bool ret = expect_dictionary_keys(
					"alert", ZERO_OR_ONE, expect_bool(assign_variable_callback(alert)),
...
					"effect", ONE_EXACTLY, effect.expect_script(),
*/

/*
	0+ generator_selector
		1 string type
		1 string name = "default"
		0+ case = { value = int }
			case = {trigger = { news_printing_count = 1 value = -999}}
		can have '@2@' ?? if generator_selector is in a pattern
*/

/*
node_callback_t Object::expect_objects(length_callback_t length_callback, callback_t<std::unique_ptr<Object>&&> callback) {
	return expect_dictionary_keys_and_length(
		length_callback,

		"EMFXActorType", ZERO_OR_MORE, _expect_instance<Object, Actor>(callback),
*/

/*
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
*/

/*
bool Actor::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"actorfile", ONE_EXACTLY,
*/

/*bool SongChanceManager::load_songs_file(ast::NodeCPtr root) {
	bool ret = true;

	ret &= expect_dictionary_reserve_length(
		song_chances,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key != "song") {
				Logger::error("Invalid song declaration ", key);
				return false;
			}
			std::string_view name {};
			ConditionalWeight chance { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };

			bool ret = expect_dictionary_keys(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
				"chance", ONE_EXACTLY, chance.expect_conditional_weight(ConditionalWeight::FACTOR)
			)(value);

			ret &= song_chances.add_item({ song_chances.size(), name, std::move(chance) });
			return ret;
		}
	)(root);

	if (song_chances.size() == 0) {
		Logger::error("No songs found in Songs.txt");
		return false;
	}

	return ret;
}

bool SongChanceManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (SongChance& songChance : song_chances.get_items()) {
		ret &= songChance.parse_scripts(definition_manager);
	}
	return ret;
}*/



/*
	"bonus", ZERO_OR_MORE, [&bonuses](ast::NodeCPtr bonus_node) -> bool {
		ConditionScript trigger { scope_t::STATE, scope_t::NO_SCOPE, scope_t::NO_SCOPE };
		fixed_point_t bonus_value {};
		const bool ret = expect_dictionary_keys(
			"trigger", ONE_EXACTLY, trigger.expect_script(),
			"value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(bonus_value))
		)(bonus_node);
		bonuses.emplace_back(std::move(trigger), bonus_value);
		return ret;
	},
*/