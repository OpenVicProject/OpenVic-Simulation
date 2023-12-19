#include "GFX.hpp"

using namespace OpenVic;
using namespace OpenVic::GFX;
using namespace OpenVic::NodeTools;

Font::Font(std::string_view new_identifier, colour_argb_t new_colour, std::string_view new_fontname)
	: HasIdentifierAndAlphaColour { new_identifier, new_colour, false }, fontname { new_fontname } {}

node_callback_t Sprite::expect_sprite(callback_t<std::unique_ptr<Sprite>&&> callback) {
	return expect_dictionary_keys(
		"spriteType", ZERO_OR_MORE, _expect_instance<Sprite, TextureSprite>(callback),
		"progressbartype", ZERO_OR_MORE, _expect_instance<Sprite, ProgressBar>(callback),
		"PieChartType", ZERO_OR_MORE, _expect_instance<Sprite, PieChart>(callback),
		"LineChartType", ZERO_OR_MORE, _expect_instance<Sprite, LineChart>(callback),
		"textSpriteType", ZERO_OR_MORE, _expect_instance<Sprite, TextureSprite>(callback),
		"maskedShieldType", ZERO_OR_MORE, _expect_instance<Sprite, MaskedFlag>(callback),
		// TODO - add the rest of the sprite types
		"corneredTileSpriteType", ZERO_OR_MORE, success_callback,
		"tileSpriteType", ZERO_OR_MORE, success_callback,
		"BarChartType", ZERO_OR_MORE, success_callback,
		"scrollingSprite", ZERO_OR_MORE, success_callback
	);
}

TextureSprite::TextureSprite() : texture_file {}, no_of_frames { NO_FRAMES } {}

bool TextureSprite::_fill_key_map(key_map_t& key_map) {
	bool ret = Sprite::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"texturefile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file)),
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file)),
		"noOfFrames", ZERO_OR_ONE, expect_uint(assign_variable_callback(no_of_frames)),

		"norefcount", ZERO_OR_ONE, success_callback,
		"effectFile", ZERO_OR_ONE, success_callback,
		"allwaystransparent", ZERO_OR_ONE, success_callback,
		"transparencecheck", ZERO_OR_ONE, success_callback,
		"loadType", ZERO_OR_ONE, success_callback,
		"clicksound", ZERO_OR_ONE, success_callback
	);
	return ret;
}

ProgressBar::ProgressBar() : back_colour {}, progress_colour {} {}

bool ProgressBar::_fill_key_map(key_map_t& key_map) {
	bool ret = Sprite::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(back_colour)),
		"colortwo", ONE_EXACTLY, expect_colour(assign_variable_callback(progress_colour)),
		"textureFile1", ZERO_OR_ONE, expect_string(assign_variable_callback_string(progress_texture_file)),
		"textureFile2", ZERO_OR_ONE, expect_string(assign_variable_callback_string(back_texture_file)),
		"size", ONE_EXACTLY, expect_ivec2(assign_variable_callback(size)),

		"effectFile", ONE_EXACTLY, success_callback,
		"allwaystransparent", ZERO_OR_ONE, success_callback,
		"loadType", ZERO_OR_ONE, success_callback,
		"horizontal", ZERO_OR_ONE, success_callback
	);
	return ret;
}

PieChart::PieChart() : size {} {}

bool PieChart::_fill_key_map(key_map_t& key_map) {
	bool ret = Sprite::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map, "size", ONE_EXACTLY, expect_uint(assign_variable_callback(size)));
	return ret;
}

LineChart::LineChart() : size {}, linewidth {} {}

bool LineChart::_fill_key_map(key_map_t& key_map) {
	bool ret = Sprite::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_ivec2(assign_variable_callback(size)),
		"linewidth", ONE_EXACTLY, expect_uint(assign_variable_callback(linewidth)),
		"allwaystransparent", ZERO_OR_ONE, success_callback
	);
	return ret;
}

MaskedFlag::MaskedFlag() : overlay_file {}, mask_file {} {}

bool MaskedFlag::_fill_key_map(key_map_t& key_map) {
	bool ret = Sprite::_fill_key_map(key_map);
	ret &= add_key_map_entries(key_map,
		"textureFile1", ONE_EXACTLY, expect_string(assign_variable_callback_string(overlay_file)),
		"textureFile2", ONE_EXACTLY, expect_string(assign_variable_callback_string(mask_file)),
		"effectFile", ONE_EXACTLY, success_callback,
		"allwaystransparent", ZERO_OR_ONE, success_callback,
		"flipv", ZERO_OR_ONE, success_callback
	);
	return ret;
}
