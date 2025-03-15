#include "GFXObject.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::GFX;
using namespace OpenVic::NodeTools;

node_callback_t Object::expect_objects(length_callback_t length_callback, callback_t<std::unique_ptr<Object>&&> callback) {
	return expect_dictionary_keys_and_length(
		length_callback,

		"EMFXActorType", ZERO_OR_MORE, _expect_instance<Object, Actor>(callback),

		/* arrows.gfx */
		"arrowType", ZERO_OR_MORE, _expect_instance<Object, ArrowType>(callback),

		/* battlearrow.gfx */
		"battlearrow", ZERO_OR_MORE, _expect_instance<Object, BattleArrow>(callback),
		"mapinfo", ZERO_OR_MORE, _expect_instance<Object, MapInfo>(callback),

		/* mapitems.gfx */
		"projectionType", ZERO_OR_MORE, _expect_instance<Object, Projection>(callback),
		"progressbar3dType", ZERO_OR_MORE, _expect_instance<Object, ProgressBar3d>(callback),
		"billboardType", ZERO_OR_MORE, _expect_instance<Object, Billboard>(callback),
		"unitstatsBillboardType", ZERO_OR_MORE, success_callback,

		/* core.gfx */
		"animatedmaptext", ZERO_OR_MORE, _expect_instance<Object, AnimatedMapText>(callback),

		"flagType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback
		),
		"provinceType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"provinceWaterType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"mapTextType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"meshType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"xfile", ONE_EXACTLY, success_callback
		)
	);
}

Actor::Attachment::Attachment(std::string_view new_actor_name, std::string_view new_attach_node, attach_id_t new_attach_id)
  : actor_name { new_actor_name }, attach_node { new_attach_node }, attach_id { new_attach_id } {}

Actor::Animation::Animation(std::string_view new_file, fixed_point_t new_scroll_time)
  : file { new_file }, scroll_time { new_scroll_time } {}

Actor::Actor() {}

bool Actor::_set_animation(std::string_view name, std::string_view file, fixed_point_t scroll_time) {
	std::optional<Animation>* animation = nullptr;

	if (name == "idle") {
		animation = &idle_animation;
	} else if (name == "move") {
		animation = &move_animation;
	} else if (name == "attack") {
		animation = &attack_animation;
	} else {
		Logger::error(
			"Unknown animation type \"", name, "\" for actor ", get_name(), " (with file ", file, " and scroll time ",
			scroll_time, ")"
		);
		return false;
	}

	if (animation->has_value()) {
		Logger::error(
			"Duplicate ", name, " animation for actor ", get_name(), ": ", file, " with scroll time ",
			scroll_time, " (already set to ", (*animation)->file, " with scroll time ", (*animation)->scroll_time, ")"
		);
		return false;
	}

	/* Don't set static/non-moving animation, to avoid needing an AnimationPlayer for the generated Godot Skeleton3D. */
	static constexpr std::string_view null_animation = "static.xsm";
	if (file.ends_with(null_animation)) {
		return true;
	}

	animation->emplace(Animation { file, scroll_time });
	return true;
}

bool Actor::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	using namespace std::string_view_literals;
	static const auto bind_set_animation = [](Actor* self, std::string_view name) {
		return [self, name](std::string_view file){ return self->_set_animation(name, file, 0); };
	};

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

		"idle", ZERO_OR_ONE, expect_string(bind_set_animation(this, "idle"sv)),
		"move", ZERO_OR_ONE, expect_string(bind_set_animation(this, "move"sv)),
		"attack", ZERO_OR_ONE, expect_string(bind_set_animation(this, "attack"sv)),
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

/* arrows.gfx */
ArrowType::ArrowType() {}

bool ArrowType::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(size)),
		"textureFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"bodytexture", ONE_EXACTLY, expect_string(assign_variable_callback_string(body_texture_file)),
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(back_colour)),
		"colortwo", ONE_EXACTLY, expect_colour(assign_variable_callback(progress_colour)),
		"endAt", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(end_at)),
		"height", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(height)),
		"type", ONE_EXACTLY, expect_int64(assign_variable_callback(arrow_type)),
		"heading", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(heading)),
		"effect", ONE_EXACTLY, expect_string(assign_variable_callback_string(effect_file))
	);

	return ret;
}

/* battlearrow.gfx */
BattleArrow::BattleArrow() {}

bool BattleArrow::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_arrow_body)),
		"textureFile1", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_arrow_head)),
		"start", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(start)),
		"stop", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(stop)),
		"x", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(dims.x)),
		"y", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(dims.y)),
		"font", ONE_EXACTLY, expect_string(assign_variable_callback_string(font)),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),
		"nofade", ZERO_OR_ONE, expect_bool(assign_variable_callback(no_fade)),
		"textureloop", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(texture_loop))
	);

	return ret;
}

MapInfo::MapInfo() {}

bool MapInfo::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file)),
		"scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(scale))
	);

	return ret;
}

/* MapItems.gfx */
Projection::Projection() {}

//TODO: Verify...
// whether pulseSpeed is fixedpoint_t or int
// pulseSpeed doesn't seem to do anything, so assume fixed_point_t since its a speed
//fadeout could be int, expect_int_bool, or fixed_point_t
//fadeout seems not to do anything
bool Projection::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"size", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(size)),
		"spin", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(spin)),
		"pulsating", ONE_EXACTLY, expect_bool(assign_variable_callback(pulsating)),
		"pulseLowest", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pulse_lowest)),
		"pulseSpeed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pulse_speed)),
		"additative", ONE_EXACTLY, expect_bool(assign_variable_callback(additative)),
		"expanding", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(expanding)),
		"duration", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(duration)),
		"fadeout", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(fadeout))
	);

	return ret;
}


Billboard::Billboard() {}

bool Billboard::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"texturefile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"noOfFrames", ZERO_OR_ONE, expect_uint((callback_t<int>)[this](frame_t frames_read) -> bool {
			if (frames_read < 1) {
				Logger::error("Billboard ", this->get_name(), " had an invalid number of frames ", frames_read, ", setting number of frames to 1");
				no_of_frames = 1;
				return false;
			}
			no_of_frames = frames_read;
			return true;
		}),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),
		"font_size", ZERO_OR_ONE, success_callback,
		"offset2", ZERO_OR_ONE, success_callback,
		"font", ZERO_OR_ONE, success_callback
	);

	return ret;
}

ProgressBar3d::ProgressBar3d() {}

bool ProgressBar3d::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(progress_colour)),
		"colortwo", ONE_EXACTLY, expect_colour(assign_variable_callback(back_colour)),
		"size", ONE_EXACTLY, expect_ivec2(assign_variable_callback(size)),
		"effectFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(effect_file))
	);

	return ret;
}


/* core.gfx */
AnimatedMapText::AnimatedMapText() {}

bool AnimatedMapText::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"speed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(speed)),
		"position", ZERO_OR_ONE, expect_fvec3(assign_variable_callback(position)),
		"scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(scale)),
		"textblock", ONE_EXACTLY, expect_dictionary_keys(
			"text", ONE_EXACTLY,  expect_string(assign_variable_callback_string(text)),
			"color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
			"font", ONE_EXACTLY,  expect_string(assign_variable_callback_string(font)),
			"position", ONE_EXACTLY, expect_fvec2(assign_variable_callback(text_position)),
			"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
			"format", ONE_EXACTLY, expect_text_format(assign_variable_callback(format))
		)
	);

	return ret;
}