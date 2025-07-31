#include "GUI.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/interface/UI.hpp"
#include "openvic-simulation/misc/SoundEffect.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

using namespace OpenVic;
using namespace OpenVic::GUI;
using namespace OpenVic::NodeTools;

Position::Position() {}

bool Position::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Named::_fill_key_map(key_map);
	ret &= add_key_map_entry(key_map,
		"position", ONE_EXACTLY, expect_fvec2(assign_variable_callback(position))
	);
	return ret;
}

Element::Element() {}

bool Element::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Named::_fill_key_map(key_map, ui_manager);
	using enum orientation_t;
	static const string_map_t<orientation_t> orientation_map = {
		{ "UPPER_LEFT", UPPER_LEFT }, { "LOWER_LEFT", LOWER_LEFT },
		{ "LOWER_RIGHT", LOWER_RIGHT }, { "UPPER_RIGHT", UPPER_RIGHT },
		{ "CENTER", CENTER }, { "CENTER_UP", CENTER_UP }, { "CENTER_DOWN", CENTER_DOWN }
	};
	ret &= add_key_map_entries(key_map,
		"position", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(position)),
		"orientation", ZERO_OR_ONE, expect_identifier_or_string(expect_mapped_string(
			orientation_map, assign_variable_callback(orientation),
			true /* Warn if the key here is invalid, leaving the default orientation UPPER_LEFT unchanged. */
		))
	);
	return ret;
}

bool Element::_fill_elements_key_map(
	NodeTools::case_insensitive_key_map_t& key_map, callback_t<memory::unique_base_ptr<Element>&&> callback, UIManager const& ui_manager
) {
	bool ret = true;
	ret &= add_key_map_entries(key_map,
		"iconType", ZERO_OR_MORE, _expect_instance<Element, Icon>(callback, ui_manager),
		"shieldtype", ZERO_OR_MORE, _expect_instance<Element, Icon>(callback, ui_manager),
		"guiButtonType", ZERO_OR_MORE, _expect_instance<Element, Button>(callback, ui_manager),
		"checkboxType", ZERO_OR_MORE, _expect_instance<Element, Checkbox>(callback, ui_manager),
		"textBoxType", ZERO_OR_MORE, _expect_instance<Element, Text>(callback, ui_manager),
		"instantTextBoxType", ZERO_OR_MORE, _expect_instance<Element, Text>(callback, ui_manager),
		"OverlappingElementsBoxType", ZERO_OR_MORE, _expect_instance<Element, OverlappingElementsBox>(callback, ui_manager),
		"listboxType", ZERO_OR_MORE, _expect_instance<Element, ListBox>(callback, ui_manager),
		"editBoxType", ZERO_OR_MORE, _expect_instance<Element, TextEditBox>(callback, ui_manager),
		"scrollbarType", ZERO_OR_MORE, _expect_instance<Element, Scrollbar>(callback, ui_manager),
		"windowType", ZERO_OR_MORE, _expect_instance<Element, Window>(callback, ui_manager),
		"eu3dialogtype", ZERO_OR_MORE, _expect_instance<Element, Window>(callback, ui_manager)
	);
	return ret;
}

bool Scene::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_elements_key_map(key_map, [this](memory::unique_base_ptr<Element>&& element) -> bool {
		return scene_elements.emplace_via_move(std::move(element));
	}, ui_manager);
	ret &= add_key_map_entry(key_map,
		"positionType", ZERO_OR_MORE, Position::_expect_value<Position>([this](Position&& position) -> bool {
			return scene_positions.emplace_via_move(std::move(position));
		})
	);
	return ret;
}

node_callback_t Scene::expect_scene(
	std::string_view scene_name, callback_t<memory::unique_base_ptr<Scene>&&> callback, UIManager const& ui_manager
) {
	return _expect_instance<Scene, Scene>([scene_name, callback](memory::unique_base_ptr<Scene>&& scene) mutable -> bool {
		scene->_set_name(scene_name);
		return callback(std::move(scene));
	}, ui_manager);
}

Window::Window() {}

bool Window::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_elements_key_map(key_map, [this](memory::unique_base_ptr<Element>&& element) -> bool {
		return window_elements.emplace_via_move(std::move(element), duplicate_warning_callback);
	}, ui_manager);
	ret &= Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"moveable", ZERO_OR_ONE, expect_int_bool(assign_variable_callback(moveable)),
		"fullScreen", ZERO_OR_ONE, expect_bool(assign_variable_callback(fullscreen)),
		"backGround", ZERO_OR_ONE, expect_string(assign_variable_callback_string(background), true),

		"dontRender", ZERO_OR_ONE, success_callback, // always empty string?
		"horizontalBorder", ZERO_OR_ONE, success_callback,
		"verticalBorder", ZERO_OR_ONE, success_callback,
		"upsound", ZERO_OR_ONE, success_callback,
		"downsound", ZERO_OR_ONE, success_callback
	);
	return ret;
}

Icon::Icon() {}

bool Icon::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		// TODO - make these share a ONE_EXACTLY count
		"spriteType", ZERO_OR_ONE, ui_manager.expect_sprite_string(assign_variable_callback_pointer(sprite)),
		"buttonMesh", ZERO_OR_ONE, ui_manager.expect_sprite_string(assign_variable_callback_pointer(sprite)),

		"frame", ZERO_OR_ONE, expect_uint(assign_variable_callback(frame)),
		"scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(scale)),
		"rotation", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(rotation))
	);
	return ret;
}

BaseButton::BaseButton() {}

bool BaseButton::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	// look up sprite registry for texture sprite with name...
	ret &= add_key_map_entries(key_map,
		// TODO - make these share a ONE_EXACTLY count
		"quadTextureSprite", ZERO_OR_ONE,
			ui_manager.expect_sprite_string(assign_variable_callback_pointer(sprite), true),
		"spriteType", ZERO_OR_ONE,
			ui_manager.expect_sprite_string(assign_variable_callback_pointer(sprite)),

		"buttonText", ZERO_OR_ONE, expect_string(assign_variable_callback_string(text), true),
		/* Some buttons have multiple fonts listed with the last one being used. */
		"buttonFont", ZERO_OR_MORE, ui_manager.expect_font_string(assign_variable_callback_pointer(font), true),
		"shortcut", ZERO_OR_ONE, expect_string(assign_variable_callback_string(shortcut), true),

		"tooltip", ZERO_OR_ONE, success_callback,
		"tooltipText", ZERO_OR_ONE, success_callback,
		"delayedTooltipText", ZERO_OR_ONE, success_callback
	);
	return ret;
}

Button::Button() {}

bool Button::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = BaseButton::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"size", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(size)),
		"rotation", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(rotation)),

		"format", ZERO_OR_ONE, success_callback, /* Is always left from what I've seen. */
		"clicksound", ZERO_OR_ONE, ui_manager.get_definition_manager().get_sound_effect_manager().expect_sound_effect_identifier_or_string(assign_variable_callback_pointer(clicksound), true, true),
		"parent", ZERO_OR_ONE, success_callback /* Links buttons to a scrollbar, not needed thanks to contextual info. */
	);
	return ret;
}

bool Checkbox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = BaseButton::_fill_key_map(key_map, ui_manager);
	return ret;
}

AlignedElement::AlignedElement() {}

bool AlignedElement::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"format", ZERO_OR_ONE, expect_text_format(assign_variable_callback(format))
	);
	return ret;
}

Text::Text() {}

bool Text::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = AlignedElement::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"text", ZERO_OR_ONE, expect_string(assign_variable_callback_string(text), true),
		"font", ONE_EXACTLY, ui_manager.expect_font_string(assign_variable_callback_pointer(font)),
		"maxWidth", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(max_size.x)),
		"maxHeight", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(max_size.y)),
		"borderSize", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(border_size)),
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file), true),

		"fixedsize", ZERO_OR_ONE, success_callback,
		"allwaystransparent", ZERO_OR_ONE, success_callback
	);
	return ret;
}

OverlappingElementsBox::OverlappingElementsBox() {}

bool OverlappingElementsBox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = AlignedElement::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"spacing", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(spacing))
	);
	return ret;
}

ListBox::ListBox() {}

bool ListBox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"offset", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(scrollbar_offset)),
		"borderSize", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(items_offset)),
		"spacing", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(spacing)),
		"scrollbartype", ZERO_OR_ONE, expect_string(assign_variable_callback_string(scrollbar_name)),

		"backGround", ZERO_OR_ONE, success_callback,
		"horizontal", ZERO_OR_ONE, success_callback,
		"priority", ZERO_OR_ONE, success_callback,
		"allwaystransparent", ZERO_OR_ONE, success_callback
	);
	return ret;
}

TextEditBox::TextEditBox() {}

bool TextEditBox::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	ret &= add_key_map_entries(key_map,
		"text", ONE_EXACTLY, expect_string(assign_variable_callback_string(text), true),
		"font", ONE_EXACTLY, ui_manager.expect_font_string(assign_variable_callback_pointer(font)),
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file), true),
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"borderSize", ONE_EXACTLY, expect_fvec2(assign_variable_callback(border_size))
	);
	return ret;
}

Scrollbar::Scrollbar() {
	scrollbar_elements.reserve(4); /* Space for 4 buttons, might need 2 more for range limit icons. */
}

bool Scrollbar::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) {
	bool ret = Element::_fill_key_map(key_map, ui_manager);
	const auto add_element = [this](memory::unique_base_ptr<Element>&& element) -> bool {
		return scrollbar_elements.emplace_via_move(std::move(element));
	};
	ret &= add_key_map_entries(key_map,
		"slider", ONE_EXACTLY, expect_string(assign_variable_callback_string(slider_button_name)),
		"track", ONE_EXACTLY, expect_string(assign_variable_callback_string(track_button_name)),
		"leftbutton", ONE_EXACTLY, expect_string(assign_variable_callback_string(less_button_name)),
		"rightbutton", ONE_EXACTLY, expect_string(assign_variable_callback_string(more_button_name)),
		"size", ONE_EXACTLY, expect_fvec2(assign_variable_callback(size)),
		"borderSize", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(border_size)),
		"minValue", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(min_value)),
		"maxValue", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_value)),
		"stepSize", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(step_size)),
		"startValue", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(start_value)),
		"horizontal", ONE_EXACTLY, expect_int_bool(assign_variable_callback(horizontal)),
		"useRangeLimit", ZERO_OR_ONE, expect_bool(assign_variable_callback(range_limited)),
		"rangeLimitMin", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(range_limit_min)),
		"rangeLimitMax", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(range_limit_max)),
		"rangeLimitMinIcon", ZERO_OR_ONE, expect_string(assign_variable_callback_string(range_limit_min_icon_name)),
		"rangeLimitMaxIcon", ZERO_OR_ONE, expect_string(assign_variable_callback_string(range_limit_max_icon_name)),

		"guiButtonType", ONE_OR_MORE, _expect_instance<Element, Button>(add_element, ui_manager),
		"iconType", ZERO_OR_MORE, _expect_instance<Element, Icon>(add_element, ui_manager),

		"priority", ZERO_OR_ONE, success_callback
	);
	return ret;
}

template<std::derived_from<Element> T>
T const* Scrollbar::get_element(std::string_view name, std::string_view type) const {
	Element const* element = scrollbar_elements.get_item_by_identifier(name);
	if (element != nullptr) {
		T const* cast_element = element->cast_to<T>();
		if (cast_element != nullptr) {
			return cast_element;
		} else {
			Logger::error(
				"GUI Scrollbar ", get_name(), " ", type, " element ", name, " has wrong type: ", element->get_type(),
				" (expected ", T::get_type_static(), ")"
			);
			return nullptr;
		}
	} else {
		Logger::error("GUI Scrollbar ", get_name(), " has no ", type, " element named ", name, "!");
		return nullptr;
	}
}

Button const* Scrollbar::get_slider_button() const {
	return get_element<Button>(slider_button_name, "slider button");
}

Button const* Scrollbar::get_track_button() const {
	return get_element<Button>(track_button_name, "track button");
}

Button const* Scrollbar::get_less_button() const {
	return get_element<Button>(less_button_name, "less button");
}

Button const* Scrollbar::get_more_button() const {
	return get_element<Button>(more_button_name, "more button");
}

Icon const* Scrollbar::get_range_limit_min_icon() const {
	return get_element<Icon>(range_limit_min_icon_name, "range limit min icon");
}

Icon const* Scrollbar::get_range_limit_max_icon() const {
	return get_element<Icon>(range_limit_max_icon_name, "range limit max icon");
}
