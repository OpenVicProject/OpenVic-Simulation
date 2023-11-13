#pragma once

#include "openvic-simulation/interface/GUI.hpp"

namespace OpenVic {

	class UIManager {

		NamedInstanceRegistry<GFX::Sprite> sprites;
		NamedInstanceRegistry<GUI::Scene, UIManager const&> scenes;
		IdentifierRegistry<GFX::Font> fonts;

		bool _load_font(ast::NodeCPtr node);

	public:
		UIManager();

		IDENTIFIER_REGISTRY_ACCESSORS(sprite)
		IDENTIFIER_REGISTRY_ACCESSORS(scene)
		IDENTIFIER_REGISTRY_ACCESSORS(font)

		bool add_font(std::string_view identifier, colour_t colour, std::string_view fontname);

		bool load_gfx_file(ast::NodeCPtr root);
		bool load_gui_file(std::string_view scene_name, ast::NodeCPtr root);
	};
}
