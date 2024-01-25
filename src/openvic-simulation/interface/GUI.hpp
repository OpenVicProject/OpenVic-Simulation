#pragma once

#include "openvic-simulation/interface/GFX.hpp"

namespace OpenVic {
	class UIManager;
}
namespace OpenVic::GUI {
	class Scene;

	class Element : public Named<UIManager const&> {
		friend class Scene;

	public:
		enum class orientation_t {
			UPPER_LEFT, LOWER_LEFT, LOWER_RIGHT, UPPER_RIGHT, CENTER
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

	class Scene : public Named<UIManager const&> {
		friend std::unique_ptr<Scene> std::make_unique<Scene>();

		NamedInstanceRegistry<Element, UIManager const&> IDENTIFIER_REGISTRY(scene_element);

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

		NamedInstanceRegistry<Element, UIManager const&> IDENTIFIER_REGISTRY(window_element);

		fvec2_t PROPERTY(size);
		bool PROPERTY(moveable);
		bool PROPERTY(fullscreen);
		// TODO - background, dontRender, horizontalBorder, verticalBorder

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

		std::string PROPERTY(text);
		GFX::Font const* PROPERTY(font);

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
			left, centre, right
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
		fvec2_t PROPERTY(max_size); // maxWidth, maxHeight

		// TODO - borderSize, fixedsize, textureFile

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

		// TODO - backGround, spacing, scrollbartype, borderSize

	protected:
		ListBox();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

	public:
		ListBox(ListBox&&) = default;
		virtual ~ListBox() = default;

		OV_DETAIL_GET_TYPE
	};
}
