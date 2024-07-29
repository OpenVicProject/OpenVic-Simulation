#pragma once

#include "openvic-simulation/interface/GFXSprite.hpp"

namespace OpenVic {
	class UIManager;
}
namespace OpenVic::GUI {
	class Scene;

	class Position final : public Named<> {
		friend class LoadBase;

		fvec2_t PROPERTY(position);

	protected:
		Position();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Position(Position&&) = default;
		virtual ~Position() = default;
	};

	class Element : public Named<UIManager const&> {
		friend class Scene;

	public:
		enum class orientation_t {
			UPPER_LEFT, LOWER_LEFT, LOWER_RIGHT, UPPER_RIGHT, CENTER, CENTER_UP, CENTER_DOWN
		};

	private:
		fvec2_t PROPERTY(position);
		orientation_t PROPERTY(orientation);

	protected:
		Element();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;
		static bool _fill_elements_key_map(
			NodeTools::case_insensitive_key_map_t& key_map, NodeTools::callback_t<std::unique_ptr<Element>&&> callback,
			UIManager const& ui_manager
		);

	public:
		Element(Element&&) = default;
		virtual ~Element() = default;

		OV_DETAIL_GET_BASE_TYPE(Element)
		OV_DETAIL_GET_TYPE
	};

	using element_instance_registry_t = NamedInstanceRegistry<Element, UIManager const&>;

	class Scene : public Named<UIManager const&> {
		friend std::unique_ptr<Scene> std::make_unique<Scene>();

		element_instance_registry_t IDENTIFIER_REGISTRY(scene_element);
		NamedRegistry<Position> IDENTIFIER_REGISTRY(scene_position);

	protected:
		Scene() = default;

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Scene(Scene&&) = default;
		virtual ~Scene() = default;

		OV_DETAIL_GET_TYPE

		static NodeTools::node_callback_t expect_scene(
			std::string_view scene_name, NodeTools::callback_t<std::unique_ptr<Scene>&&> callback, UIManager const& ui_manager
		);

	};

	class Window final : public Element {
		friend std::unique_ptr<Window> std::make_unique<Window>();

		element_instance_registry_t IDENTIFIER_REGISTRY(window_element);

		std::string PROPERTY(background); /* The name of a child button who's sprite is used as the background. */
		fvec2_t PROPERTY(size);
		bool PROPERTY(moveable);
		bool PROPERTY(fullscreen);

		// TODO - dontRender, horizontalBorder, verticalBorder

	protected:
		Window();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Window(Window&&) = default;
		virtual ~Window() = default;

		OV_DETAIL_GET_TYPE
	};

	class Icon final : public Element {
		friend std::unique_ptr<Icon> std::make_unique<Icon>();

		GFX::Sprite const* PROPERTY(sprite);
		GFX::frame_t PROPERTY(frame);
		fixed_point_t PROPERTY(scale);
		fixed_point_t PROPERTY(rotation); /* In radians, usually one of 0, PI/2 or -PI/2. */

	protected:
		Icon();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Icon(Icon&&) = default;
		virtual ~Icon() = default;

		OV_DETAIL_GET_TYPE
	};

	class BaseButton : public Element {
		GFX::Sprite const* PROPERTY(sprite);
		std::string PROPERTY(text);
		GFX::Font const* PROPERTY(font);

		// TODO - shortcut

	protected:
		BaseButton();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		BaseButton(BaseButton&&) = default;
		virtual ~BaseButton() = default;

		OV_DETAIL_GET_TYPE
	};

	class Button final : public BaseButton {
		friend std::unique_ptr<Button> std::make_unique<Button>();

		fvec2_t PROPERTY(size);
		fixed_point_t PROPERTY(rotation); /* In radians, usually one of 0, PI/2 or -PI/2. */

		// TODO - clicksound

	protected:
		Button();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Button(Button&&) = default;
		virtual ~Button() = default;

		OV_DETAIL_GET_TYPE
	};

	class Checkbox final : public BaseButton {
		friend std::unique_ptr<Checkbox> std::make_unique<Checkbox>();

	protected:
		Checkbox() = default;

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Checkbox(Checkbox&&) = default;
		virtual ~Checkbox() = default;

		OV_DETAIL_GET_TYPE
	};

	class AlignedElement : public Element {
	public:
		enum class format_t {
			left, centre, right, justified
		};

	private:
		format_t PROPERTY(format);

	protected:
		AlignedElement();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		AlignedElement(AlignedElement&&) = default;
		virtual ~AlignedElement() = default;

		OV_DETAIL_GET_TYPE
	};

	class Text final : public AlignedElement {
		friend std::unique_ptr<Text> std::make_unique<Text>();

		std::string PROPERTY(text);
		GFX::Font const* PROPERTY(font);
		fvec2_t PROPERTY(max_size); /* Defines keys: maxWidth, maxHeight */
		fvec2_t PROPERTY(border_size);

		// TODO - fixedsize, textureFile

	protected:
		Text();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Text(Text&&) = default;
		virtual ~Text() = default;

		OV_DETAIL_GET_TYPE
	};

	class OverlappingElementsBox final : public AlignedElement {
		friend std::unique_ptr<OverlappingElementsBox> std::make_unique<OverlappingElementsBox>();

		fvec2_t PROPERTY(size);
		fixed_point_t PROPERTY(spacing);

	protected:
		OverlappingElementsBox();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		OverlappingElementsBox(OverlappingElementsBox&&) = default;
		virtual ~OverlappingElementsBox() = default;

		OV_DETAIL_GET_TYPE
	};

	class ListBox final : public Element {
		friend std::unique_ptr<ListBox> std::make_unique<ListBox>();

		fvec2_t PROPERTY(size);
		fvec2_t PROPERTY(scrollbar_offset);
		fvec2_t PROPERTY(items_offset);
		fixed_point_t PROPERTY(spacing);
		std::string PROPERTY(scrollbar_name); /* In vanilla this is always core's standardlistbox_slider */

		// TODO - backGround

	protected:
		ListBox();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		ListBox(ListBox&&) = default;
		virtual ~ListBox() = default;

		OV_DETAIL_GET_TYPE
	};

	class TextEditBox final : public Element {
		friend std::unique_ptr<TextEditBox> std::make_unique<TextEditBox>();

		std::string PROPERTY(text);
		GFX::Font const* PROPERTY(font);
		std::string PROPERTY(texture_file);
		fvec2_t PROPERTY(size);
		fvec2_t PROPERTY(border_size);

	protected:
		TextEditBox();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		TextEditBox(TextEditBox&&) = default;
		virtual ~TextEditBox() = default;

		OV_DETAIL_GET_TYPE
	};

	class Scrollbar final : public Element {
		friend std::unique_ptr<Scrollbar> std::make_unique<Scrollbar>();

		element_instance_registry_t IDENTIFIER_REGISTRY(scrollbar_element);

		std::string PROPERTY(slider_button_name);
		std::string PROPERTY(track_button_name);
		std::string PROPERTY(less_button_name);
		std::string PROPERTY(more_button_name);

		fvec2_t PROPERTY(size);
		fvec2_t PROPERTY(border_size);
		fixed_point_t PROPERTY(min_value);
		fixed_point_t PROPERTY(max_value);
		fixed_point_t PROPERTY(step_size);
		fixed_point_t PROPERTY(start_value);
		bool PROPERTY_CUSTOM_PREFIX(horizontal, is)

		bool PROPERTY_CUSTOM_PREFIX(range_limited, is);
		fixed_point_t PROPERTY(range_limit_min);
		fixed_point_t PROPERTY(range_limit_max);
		std::string PROPERTY(range_limit_min_icon_name);
		std::string PROPERTY(range_limit_max_icon_name);

		template<std::derived_from<Element> T>
		T const* get_element(std::string_view name, std::string_view type) const;

	protected:
		Scrollbar();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		Scrollbar(Scrollbar&&) = default;
		virtual ~Scrollbar() = default;

		Button const* get_slider_button() const; /* The button you grab and move up and down the scrollbar. */
		Button const* get_track_button() const; /* The track/background the slider button moves along. */
		Button const* get_less_button() const;
		Button const* get_more_button() const;

		Icon const* get_range_limit_min_icon() const;
		Icon const* get_range_limit_max_icon() const;

		OV_DETAIL_GET_TYPE
	};
}
