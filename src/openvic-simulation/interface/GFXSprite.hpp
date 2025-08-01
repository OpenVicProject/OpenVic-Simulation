#pragma once

#include "openvic-simulation/interface/LoadBase.hpp"

namespace OpenVic::GFX {

	struct Font : HasIdentifierAndAlphaColour {
		using colour_codes_t = ordered_map<char, colour_t>;

	private:
		memory::string PROPERTY(fontname);
		memory::string PROPERTY(charset);
		uint32_t PROPERTY(height);
		colour_codes_t PROPERTY(colour_codes);

		// TODO - effect

	public:
		Font(
			std::string_view new_identifier, colour_argb_t new_colour, std::string_view new_fontname,
			std::string_view new_charset, uint32_t new_height, colour_codes_t&& new_colour_codes
		);
		Font(Font&&) = default;
	};

	using frame_t = int32_t; /* Keep this as int32_t to simplify interfacing with Godot */
	static constexpr frame_t NO_FRAMES = 0;

	class Sprite : public Named<> {
	protected:
		constexpr Sprite() {};

	public:
		Sprite(Sprite&&) = default;
		virtual ~Sprite() = default;

		OV_DETAIL_GET_BASE_TYPE(Sprite)
		OV_DETAIL_GET_TYPE

		static NodeTools::node_callback_t expect_sprites(
			NodeTools::length_callback_t length_callback, NodeTools::callback_t<memory::unique_base_ptr<Sprite>&&> callback
		);
	};

	class TextureSprite : public Sprite {
		memory::string PROPERTY(texture_file);

	protected:
		TextureSprite();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		TextureSprite(TextureSprite&&) = default;
		virtual ~TextureSprite() = default;

		OV_DETAIL_GET_BASE_TYPE(TextureSprite)
		OV_DETAIL_GET_TYPE
	};

	class IconTextureSprite final : public TextureSprite {
		frame_t PROPERTY(no_of_frames, NO_FRAMES);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		IconTextureSprite();
		IconTextureSprite(IconTextureSprite&&) = default;
		virtual ~IconTextureSprite() = default;

		OV_DETAIL_GET_TYPE
	};

	class TileTextureSprite final : public TextureSprite {
		ivec2_t PROPERTY(size);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		TileTextureSprite();
		TileTextureSprite(TileTextureSprite&&) = default;
		virtual ~TileTextureSprite() = default;

		OV_DETAIL_GET_TYPE
	};

	class CorneredTileTextureSprite final : public TextureSprite {
		ivec2_t PROPERTY(size);
		ivec2_t PROPERTY(border_size);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		CorneredTileTextureSprite();
		CorneredTileTextureSprite(CorneredTileTextureSprite&&) = default;
		virtual ~CorneredTileTextureSprite() = default;

		OV_DETAIL_GET_TYPE
	};

	class ProgressBar final : public Sprite {
		colour_t PROPERTY(back_colour);
		memory::string PROPERTY(back_texture_file);
		colour_t PROPERTY(progress_colour);
		memory::string PROPERTY(progress_texture_file);
		ivec2_t PROPERTY(size);

		// TODO - effectFile

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		ProgressBar();
		ProgressBar(ProgressBar&&) = default;
		virtual ~ProgressBar() = default;

		OV_DETAIL_GET_TYPE
	};

	class PieChart final : public Sprite {
		uint32_t PROPERTY(size, 0);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		PieChart();
		PieChart(PieChart&&) = default;
		virtual ~PieChart() = default;

		OV_DETAIL_GET_TYPE
	};

	class LineChart final : public Sprite {
		ivec2_t PROPERTY(size);
		uint32_t PROPERTY(linewidth, 0);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		LineChart();
		LineChart(LineChart&&) = default;
		virtual ~LineChart() = default;

		OV_DETAIL_GET_TYPE
	};

	class MaskedFlag final : public Sprite {
		memory::string PROPERTY(overlay_file);
		memory::string PROPERTY(mask_file);

	protected:
		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		MaskedFlag();
		MaskedFlag(MaskedFlag&&) = default;
		virtual ~MaskedFlag() = default;

		OV_DETAIL_GET_TYPE
	};
}
