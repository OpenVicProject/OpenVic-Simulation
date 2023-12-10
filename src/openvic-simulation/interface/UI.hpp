#pragma once

#include "openvic-simulation/interface/GUI.hpp"

namespace OpenVic {

	class UIManager { 
		NamedInstanceRegistry<GFX::Sprite> IDENTIFIER_REGISTRY(sprite);
		NamedInstanceRegistry<GUI::Scene, UIManager const&> IDENTIFIER_REGISTRY(scene);
		IdentifierRegistry<GFX::Font> IDENTIFIER_REGISTRY(font);

		bool _load_font(ast::NodeCPtr node);

	public:
		bool add_font(std::string_view identifier, colour_t colour, std::string_view fontname);

		bool load_gfx_file(ast::NodeCPtr root);
		bool load_gui_file(std::string_view scene_name, ast::NodeCPtr root);
	};
}
