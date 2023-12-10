#include "UI.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace OpenVic::GFX;
using namespace OpenVic::GUI;

bool UIManager::add_font(std::string_view identifier, colour_t colour, std::string_view fontname) {
	if (identifier.empty()) {
		Logger::error("Invalid font identifier - empty!");
		return false;
	}
	if (fontname.empty()) {
		Logger::error("Invalid culture colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	return fonts.add_item({ identifier, colour, fontname }, duplicate_warning_callback);
}

bool UIManager::_load_font(ast::NodeCPtr node) {
	std::string_view identifier, fontname;
	colour_t colour = NULL_COLOUR;
	bool ret = expect_dictionary_keys(
		"name", ONE_EXACTLY, expect_string(assign_variable_callback(identifier)),
		"fontname", ONE_EXACTLY, expect_string(assign_variable_callback(fontname)),
		"color", ONE_EXACTLY, expect_colour_hex(assign_variable_callback(colour)),
		"colorcodes", ZERO_OR_ONE, success_callback,
		"effect", ZERO_OR_ONE, success_callback
	)(node);
	ret &= add_font(identifier, colour, fontname);
	return ret;
}

bool UIManager::load_gfx_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"spriteTypes", ZERO_OR_ONE, Sprite::expect_sprite(
			[this](std::unique_ptr<Sprite>&& sprite) -> bool {
				/* TODO - more checks on duplicates (the false here reduces them from
				 * errors to warnings). The repeats in vanilla are identical (simple
				 * texture sprites with the same name referring to the same asset),
				 * maybe check if duplicates are equal? (this would require some kind
				 * of duplicate handling callback for `add_item`)
				 *
				 * It's also possible that some of the vanilla duplicates are intentional,
				 * after all I'm assuming all GFX files are loaded into a global registry
				 * rather than each GUI file using only the contnts of a few specific,
				 * maybe even hardcoded GFX files. This can't, however, explain all of the
				 * vanilla duplicates, as GFX_frontend_tab_button is repeated in the same
				 * file, albeit in a section marked "OLD STUFF BELOW..".
				 *
				 * Vanilla Duplicates:
				 * - GFX_bp_open_manager and GFX_bp_toggle_visible
				 *   - simple texture sprites appearing identically in menubar.gfx and battleplansinterface.gfx
				 * - GFX_frontend_tab_button
				 *   - texture sprite appearing twice identically in the same file (temp_frontend.gfx),
				 *     both below a comment saying "OLD STUFF BELOW.."
				 */
				return sprites.add_item(std::move(sprite), duplicate_warning_callback);
			}
		),
		"bitmapfonts", ZERO_OR_ONE, expect_dictionary(
			[this](std::string_view key, ast::NodeCPtr node) -> bool {
				if (key != "bitmapfont") {
					Logger::error("Invalid bitmapfonts key: ", key);
					return false;
				}
				return _load_font(node);
			}
		),
		"objectTypes", ZERO_OR_ONE, success_callback,
		"lightTypes", ZERO_OR_ONE, success_callback,
		"fonts", ZERO_OR_ONE, success_callback
	)(root);
}

bool UIManager::load_gui_file(std::string_view scene_name, ast::NodeCPtr root) {
	if (!sprites_are_locked()) {
		Logger::error("Cannot load GUI files until GFX files (i.e. Sprites) are locked!");
		return false;
	}
	return expect_dictionary_keys(
		"guiTypes", ZERO_OR_ONE, Scene::expect_scene(
			scene_name,
			[this](std::unique_ptr<Scene>&& scene) -> bool {
				return scenes.add_item(std::move(scene));
			},
			*this
		)
	)(root);
}
