#include "UI.hpp"

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace OpenVic::GFX;
using namespace OpenVic::GUI;

bool UIManager::add_font(
	std::string_view identifier, colour_argb_t colour, std::string_view fontname, std::string_view charset, uint32_t height,
	Font::colour_codes_t&& colour_codes
) {
	if (identifier.empty()) {
		Logger::error("Invalid font identifier - empty!");
		return false;
	}
	if (colour.alpha == colour_argb_t::colour_traits::null) {
		Logger::error("Invalid colour for font ", identifier, " - completely transparent! (", colour, ")");
		return false;
	}
	if (fontname.empty()) {
		Logger::error("Invalid fontname for font ", identifier, " - empty!");
		return false;
	}
	const bool ret = fonts.add_item(
		{ identifier, colour, fontname, charset, height, std::move(colour_codes) },
		duplicate_warning_callback
	);

	if (universal_colour_codes.empty() && ret) {
		GFX::Font::colour_codes_t const& loaded_colour_codes = get_fonts().back().get_colour_codes();
		if (!loaded_colour_codes.empty()) {
			universal_colour_codes = loaded_colour_codes;
			Logger::info("Loaded universal colour codes from font: \"", identifier, "\"");
		}
	}

	return ret;
}

bool UIManager::_load_font(ast::NodeCPtr node) {
	std::string_view identifier, fontname, charset;
	colour_argb_t colour = colour_argb_t::null();
	uint32_t height = 0;
	Font::colour_codes_t colour_codes;

	bool ret = expect_dictionary_keys(
		"name", ONE_EXACTLY, expect_string(assign_variable_callback(identifier)),
		"fontname", ONE_EXACTLY, expect_string(assign_variable_callback(fontname)),
		"color", ONE_EXACTLY, expect_colour_hex(assign_variable_callback(colour)),
		"charset", ZERO_OR_ONE, expect_string(assign_variable_callback(charset)),
		"height", ZERO_OR_ONE, expect_uint(assign_variable_callback(height)),
		"colorcodes", ZERO_OR_ONE, expect_dictionary(
			[&colour_codes](std::string_view key, ast::NodeCPtr value) -> bool {
				if (key.size() != 1) {
					Logger::error("Invalid colour code key: \"", key, "\" (expected single character)");
					return false;
				}
				return expect_colour(map_callback(colour_codes, key.front()))(value);
			}
		),
		"effect", ZERO_OR_ONE, success_callback
	)(node);

	ret &= add_font(identifier, colour, fontname, charset, height, std::move(colour_codes));

	return ret;
}

NodeCallback auto UIManager::_load_fonts(std::string_view font_key) {
	return expect_dictionary_reserve_length(
		fonts,
		[this, font_key](std::string_view key, ast::NodeCPtr node) -> bool {
			if (key != font_key) {
				Logger::error("Invalid key: \"", key, "\" (expected ", font_key, ")");
				return false;
			}
			return _load_font(node);
		}
	);
}

void UIManager::lock_gfx_registries() {
	lock_sprites();
	lock_fonts();
	lock_objects();
}

bool UIManager::load_gfx_file(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"spriteTypes", ZERO_OR_ONE, Sprite::expect_sprites(
			NodeTools::reserve_length_callback(sprites),
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

		"bitmapfonts", ZERO_OR_ONE, _load_fonts("bitmapfont"),
		"fonts", ZERO_OR_ONE, _load_fonts("font"),

		"objectTypes", ZERO_OR_ONE, Object::expect_objects(
			NodeTools::reserve_length_callback(objects),
			[this](std::unique_ptr<Object>&& object) -> bool {
				/* There are various models with the same name but slight differences, e.g. Prussian and German variants
				 * of PrussianGCCavalry (the latter added in a spritepack). Currently we default to using the first loaded
				 * model of each name, but we may want to switch to using the last loaded or allow multiple models per name
				 * (e.g. by grouping them per gfx file). */
				return objects.add_item(std::move(object), duplicate_warning_callback);
			}
		),

		"lightTypes", ZERO_OR_ONE, success_callback
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
