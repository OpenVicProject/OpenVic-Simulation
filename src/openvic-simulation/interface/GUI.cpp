#include "GUI.hpp"

#include "openvic-simulation/interface/UI.hpp"

using namespace OpenVic;
using namespace OpenVic::GUI;
using namespace OpenVic::NodeTools;

Element::Element() : position {}, orientation { orientation_t::UPPER_LEFT } {}

bool Element::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Named::_fill_key_map(key_map, ui_manager);
	using enum orientation_t;
	static const string_map_t<orientation_t> orientation_map = {
		{ "UPPER_LEFT", UPPER_LEFT }, { "LOWER_LEFT", LOWER_LEFT },
		{ "LOWER_RIGHT", LOWER_RIGHT }, { "UPPER_RIGHT", UPPER_RIGHT },
		{ "CENTER", CENTER }
	};
	ret &= add_key_map_entries(key_map,
		"position", ONE_EXACTLY, expect_fvec2(assign_variable_callback(position)),
		"orientation", ZERO_OR_ONE, expect_string(expect_mapped_string(orientation_map, assign_variable_callback(orientation)))
	);
	return ret;
}

bool Element::_fill_elements_key_map(
	NodeTools::case_insensitive_key_map_t& key_map, callback_t<std::unique_ptr<Element>&&> callback, UIManager const& ui_manager
) {
	bool ret = true;
	ret &= add_key_map_entries(key_map,
		"iconType", ZERO_OR_MORE, _expect_instance<Element, Icon>(callback, ui_manager),
		"guiButtonType", ZERO_OR_MORE, _expect_instance<Element, Button>(callback, ui_manager),
		"checkboxType", ZERO_OR_MORE, _expect_instance<Element, Checkbox>(callback, ui_manager),
		"textBoxType", ZERO_OR_MORE, _expect_instance<Element, Text>(callback, ui_manager),
		"instantTextBoxType", ZERO_OR_MORE, _expect_instance<Element, Text>(callback, ui_manager),
		"OverlappingElementsBoxType", ZERO_OR_MORE, _expect_instance<Element, OverlappingElementsBox>(callback, ui_manager),
		"listboxType", ZERO_OR_MORE, _expect_instance<Element, ListBox>(callback, ui_manager),
		"windowType", ZERO_OR_MORE, _expect_instance<Element, Window>(callback, ui_manager),
		"positionType", ZERO_OR_MORE, success_callback // TODO - load this as a marker for placing sub-scenes
	);
	return ret;
}

bool Scene::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	return Element::_fill_elements_key_map(key_map, [this](std::unique_ptr<Element>&& element) -> bool {
		return scene_elements.add_item(std::move(element));
	}, ui_manager);
}

node_callback_t Scene::expect_scene(
	std::string_view scene_name, callback_t<std::unique_ptr<Scene>&&> callback, UIManager const& ui_manager
) {
	return _expect_instance<Scene, Scene>([scene_name, callback](std::unique_ptr<Scene>&& scene) -> bool {
		scene->_set_name(scene_name);
		return callback(std::move(scene));
	}, ui_manager);
}

Window::Window() : moveable { false }, fullscreen { false } {}

bool Window::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_elements_key_map(key_map, [this](std::unique_ptr<Element>&& element) -> bool {
		return window_elements.add_item(std::move(element));
	}, ui_manager);
	ret &= Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"backGround", ZERO_OR_ONE, success_callback, // TODO - load as potential panel texture (almost always empty)
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"moveable", ONE_EXACTLY, expect_int_bool(assign_variable_callback(moveable)),
		"dontRender", ZERO_OR_ONE, success_callback, // always empty string?
		"horizontalBorder", ZERO_OR_ONE, success_callback,
		"verticalBorder", ZERO_OR_ONE, success_callback,
		"fullScreen", ZERO_OR_ONE, expect_bool(assign_variable_callback(fullscreen))
	);
	return ret;
}

Icon::Icon() : sprite { nullptr }, frame { GFX::NO_FRAMES } {}

bool Icon::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"spriteType", ONE_EXACTLY, expect_string(ui_manager.expect_sprite_str(assign_variable_callback_pointer(sprite))),
		"frame", ZERO_OR_ONE, expect_uint(assign_variable_callback(frame))
	);
	return ret;
}

BaseButton::BaseButton() : sprite { nullptr } {}

bool BaseButton::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	// look up sprite registry for texture sprite with name...
	ret &= add_key_map_entries(key_map,
		"quadTextureSprite", ZERO_OR_ONE,
			expect_string(ui_manager.expect_sprite_str(assign_variable_callback_pointer(sprite)), true),
		"spriteType", ZERO_OR_ONE,
			expect_string(ui_manager.expect_sprite_str(assign_variable_callback_pointer(sprite)), true),
		"shortcut", ZERO_OR_ONE, success_callback // TODO - load and use shortcuts (how to integrate with custom keybinds?)
	);
	return ret;
}

Button::Button() : text {}, font { nullptr} {}

bool Button::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = BaseButton::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"buttonText", ZERO_OR_ONE, expect_string(assign_variable_callback_string(text), true),
		"buttonFont", ZERO_OR_ONE, expect_string(ui_manager.expect_font_str(assign_variable_callback_pointer(font))),
		"clicksound", ZERO_OR_ONE, success_callback,
		/* These are always empty in the base defines */
		"tooltip", ZERO_OR_ONE, success_callback,
		"tooltipText", ZERO_OR_ONE, success_callback,
		"delayedTooltipText", ZERO_OR_ONE, success_callback
	);
	return ret;
}

bool Checkbox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = BaseButton::_fill_key_map(key_map, ui_manager);
	return ret;
}

AlignedElement::AlignedElement() : format { format_t::left } {}

bool AlignedElement::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	using enum format_t;
	static const string_map_t<format_t> format_map = {
		{ "left", left }, { "right", right }, { "centre", centre }, { "center", centre }
	};
	ret &= add_key_map_entries(key_map,
		"format", ZERO_OR_ONE, expect_identifier(expect_mapped_string(format_map, assign_variable_callback(format))
	));
	return ret;
}

Text::Text() : text {}, font { nullptr } {}

bool Text::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = AlignedElement::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"text", ZERO_OR_ONE, expect_string(assign_variable_callback_string(text), true),
		"font", ONE_EXACTLY, expect_string(ui_manager.expect_font_str(assign_variable_callback_pointer(font))),
		"maxWidth", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_size.x)),
		"maxHeight", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_size.y)),

		"borderSize", ZERO_OR_ONE, success_callback,
		"fixedsize", ZERO_OR_ONE, success_callback,

		// Add warning about redundant key?
		"textureFile", ZERO_OR_ONE, success_callback
	);
	return ret;
}

OverlappingElementsBox::OverlappingElementsBox() : size {} {}

bool OverlappingElementsBox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = AlignedElement::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"spacing", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(spacing))
	);
	return ret;
}

ListBox::ListBox() : size {} {}

bool ListBox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"backGround", ZERO_OR_ONE, success_callback,
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"spacing", ZERO_OR_ONE, success_callback,
		"scrollbartype", ZERO_OR_ONE, success_callback, // TODO - implement modable listbox scrollbars
		"borderSize", ZERO_OR_ONE, success_callback
	);
	return ret;
}
