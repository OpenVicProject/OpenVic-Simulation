#pragma once

#include "openvic-simulation/interface/LoadBase.hpp"

namespace OpenVic {
	class UIManager;
}

namespace OpenVic::GFX {

	struct Font : HasIdentifierAndColour {
		friend class OpenVic::UIManager;

	private:
		const std::string PROPERTY(fontname);

		// TODO - colorcodes, effect

		Font(std::string_view new_identifier, colour_t new_colour, std::string_view new_fontname);

	public:
		Font(Font&&) = default;
	};

	using frame_t = int32_t; /* Keep this as int32_t to simplify interfacing with Godot */
	static constexpr frame_t NO_FRAMES = 0;

	class Sprite : public Named<> {
	protected:
		Sprite() = default;

	public:
		Sprite(Sprite&&) = default;
		virtual ~Sprite() = default;

		OV_DETAIL_GET_BASE_TYPE(Sprite)
		OV_DETAIL_GET_TYPE

		static NodeTools::node_callback_t expect_sprite(NodeTools::callback_t<std::unique_ptr<Sprite>&&> callback);
	};

	class TextureSprite final : public Sprite {
		friend std::unique_ptr<TextureSprite> std::make_unique<TextureSprite>();

		std::string PROPERTY(texture_file);
		frame_t PROPERTY(no_of_frames);

		// TODO - norefcount, effectFile, allwaystransparent

	protected:
		TextureSprite();

		bool _fill_key_map(NodeTools::key_map_t& key_map) override;

	public:
		TextureSprite(TextureSprite&&) = default;
		virtual ~TextureSprite() = default;

		OV_DETAIL_GET_TYPE
	};

	class ProgressBar final : public Sprite {
		friend std::unique_ptr<ProgressBar> std::make_unique<ProgressBar>();

		colour_t PROPERTY(back_colour);
		std::string PROPERTY(back_texture_file);
		colour_t PROPERTY(progress_colour);
		std::string PROPERTY(progress_texture_file);
		ivec2_t PROPERTY(size);

		// TODO - effectFile

	protected:
		ProgressBar();

		bool _fill_key_map(NodeTools::key_map_t& key_map) override;

	public:
		ProgressBar(ProgressBar&&) = default;
		virtual ~ProgressBar() = default;

		OV_DETAIL_GET_TYPE
	};

	class PieChart final : public Sprite {
		friend std::unique_ptr<PieChart> std::make_unique<PieChart>();

		uint32_t PROPERTY(size);

	protected:
		PieChart();

		bool _fill_key_map(NodeTools::key_map_t& key_map) override;

	public:
		PieChart(PieChart&&) = default;
		virtual ~PieChart() = default;

		OV_DETAIL_GET_TYPE
	};

	class LineChart final : public Sprite {
		friend std::unique_ptr<LineChart> std::make_unique<LineChart>();

		ivec2_t PROPERTY(size);
		uint32_t PROPERTY(linewidth);

	protected:
		LineChart();

		bool _fill_key_map(NodeTools::key_map_t& key_map) override;

	public:
		LineChart(LineChart&&) = default;
		virtual ~LineChart() = default;

		OV_DETAIL_GET_TYPE
	};


	class MaskedFlag final : public Sprite {
		friend std::unique_ptr<MaskedFlag> std::make_unique<MaskedFlag>();

		std::string PROPERTY(texture_file)
		std::string PROPERTY(mask_file);

	protected:
		MaskedFlag();

		bool _fill_key_map(NodeTools::key_map_t& key_map) override;

	public:
		MaskedFlag(MaskedFlag&&) = default;
		virtual ~MaskedFlag() = default;

		OV_DETAIL_GET_TYPE
	};
}
