#include "GFXObject.hpp"

using namespace OpenVic;
using namespace OpenVic::GFX;
using namespace OpenVic::NodeTools;

node_callback_t Object::expect_objects(length_callback_t length_callback, callback_t<std::unique_ptr<Object>&&> callback) {
	return expect_dictionary_keys_and_length(
		length_callback,

		"EMFXActorType", ZERO_OR_MORE, _expect_instance<Object, Actor>(callback),

		/* arrows.gfx */
		"arrowType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"bodytexture", ONE_EXACTLY, success_callback,
			"color", ONE_EXACTLY, success_callback,
			"colortwo", ONE_EXACTLY, success_callback,
			"endAt", ONE_EXACTLY, success_callback,
			"height", ONE_EXACTLY, success_callback,
			"type", ONE_EXACTLY, success_callback,
			"heading", ONE_EXACTLY, success_callback,
			"effect", ONE_EXACTLY, success_callback
		),

		/* battlearrow.gfx */
		"battlearrow", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"textureFile1", ONE_EXACTLY, success_callback,
			"start", ONE_EXACTLY, success_callback,
			"stop", ONE_EXACTLY, success_callback,
			"x", ONE_EXACTLY, success_callback,
			"y", ONE_EXACTLY, success_callback,
			"font", ONE_EXACTLY, success_callback,
			"scale", ONE_EXACTLY, success_callback,
			"nofade", ZERO_OR_ONE, success_callback,
			"textureloop", ZERO_OR_ONE, success_callback
		),
		"mapinfo", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"textureFile", ZERO_OR_ONE, success_callback,
			"scale", ZERO_OR_ONE, success_callback
		),

		/* mapitems.gfx */
		"projectionType", ZERO_OR_MORE, success_callback,
		"progressbar3dType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"color", ONE_EXACTLY, success_callback,
			"colortwo", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback,
			"effectFile", ONE_EXACTLY, success_callback
		),
		"billboardType", ZERO_OR_MORE, expect_dictionary_keys<StringMapCaseInsensitive>(
			"name", ONE_EXACTLY, success_callback,
			"texturefile", ONE_EXACTLY, success_callback,
			"noOfFrames", ZERO_OR_ONE, success_callback,
			"scale", ONE_EXACTLY, success_callback,
			"font_size", ZERO_OR_ONE, success_callback,
			"offset2", ZERO_OR_ONE, success_callback,
			"font", ZERO_OR_ONE, success_callback
		),

		"unitstatsBillboardType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"mask", ONE_EXACTLY, success_callback,
			"effectFile", ONE_EXACTLY, success_callback,
			"scale", ONE_EXACTLY, success_callback,
			"noOfFrames", ONE_EXACTLY, success_callback,
			"font_size", ONE_EXACTLY, success_callback,
			"font", ONE_EXACTLY, success_callback
		),

		/* core.gfx */
		"animatedmaptext", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"speed", ONE_EXACTLY, success_callback,
			"position", ZERO_OR_ONE, success_callback,
			"scale", ZERO_OR_ONE, success_callback,
			"textblock", ONE_EXACTLY, expect_dictionary_keys(
				"text", ONE_EXACTLY, success_callback,
				"color", ONE_EXACTLY, success_callback,
				"font", ONE_EXACTLY, success_callback,
				"position", ONE_EXACTLY, success_callback,
				"size", ONE_EXACTLY, success_callback,
				"format", ONE_EXACTLY, success_callback
			)
		),
		"flagType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback
		),
		"provinceType", ZERO_OR_ONE, success_callback,
		"provinceWaterType", ZERO_OR_ONE, success_callback,
		"mapTextType", ZERO_OR_MORE, success_callback,
		"meshType", ZERO_OR_MORE, success_callback
	);
}

Actor::Attachment::Attachment() : node {}, attach_id { 0 } {}

bool Actor::Attachment::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Named::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"node", ONE_EXACTLY, expect_string(assign_variable_callback_string(node)),
		"attachId", ONE_EXACTLY, expect_uint(assign_variable_callback(attach_id))
	);
	return ret;
}

Actor::Animation::Animation() : file {}, default_time { 0 } {}

bool Actor::Animation::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Named::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"file", ONE_EXACTLY, expect_string(assign_variable_callback_string(file)),
		"defaultAnimationTime", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(default_time))
	);
	return ret;
}

Actor::Actor() : model_file {}, idle_animation_file {}, move_animation_file {}, attack_animation_file {}, scale { 1 } {}

bool Actor::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"actorfile", ONE_EXACTLY, expect_string(assign_variable_callback_string(model_file)),
		"idle", ZERO_OR_ONE, expect_string(assign_variable_callback_string(idle_animation_file)),
		"move", ZERO_OR_ONE, expect_string(assign_variable_callback_string(move_animation_file)),
		"attack", ZERO_OR_ONE, expect_string(assign_variable_callback_string(attack_animation_file)),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),
		"attach", ZERO_OR_MORE, Attachment::_expect_value<Attachment>([this](Attachment&& attachment) -> bool {
			return attachments.add_item(std::move(attachment));
		}),
		"animation", ZERO_OR_MORE, Animation::_expect_value<Animation>([this](Animation&& animation) -> bool {
			return animations.add_item(std::move(animation));
		})
	);
	return ret;
}
