#pragma once

#include <cstdint>
#include "openvic-simulation/interface/LoadBase.hpp"
#include "openvic-simulation/types/TextFormat.hpp"
#include "openvic-simulation/types/Vector.hpp"

namespace OpenVic::GFX {

	class Object : public Named<> {
	protected:
		Object() = default;

	public:
		Object(Object&&) = default;
		virtual ~Object() = default;

		OV_DETAIL_GET_BASE_TYPE(Object)
		OV_DETAIL_GET_TYPE

		static NodeTools::node_callback_t expect_objects(
			NodeTools::length_callback_t length_callback, NodeTools::callback_t<std::unique_ptr<Object>&&> callback
		);
	};

	class Actor final : public Object {
		friend std::unique_ptr<Actor> std::make_unique<Actor>();

	public:
		class Attachment {
			friend class Actor;

		public:
			using attach_id_t = uint32_t;

		private:
			std::string PROPERTY(actor_name);
			std::string PROPERTY(attach_node);
			attach_id_t PROPERTY(attach_id);

		public:
			Attachment(std::string_view new_actor_name, std::string_view new_attach_node, attach_id_t new_attach_id);
			Attachment(Attachment&&) = default;
		};

		class Animation {
			friend class Actor;

			std::string PROPERTY(file);
			fixed_point_t PROPERTY(scroll_time);

		public:
			Animation(std::string_view new_file, fixed_point_t new_scroll_time);
			Animation(Animation&&) = default;
		};

	private:
		fixed_point_t PROPERTY(scale, 1);
		std::string PROPERTY(model_file);
		std::optional<Animation> PROPERTY(idle_animation);
		std::optional<Animation> PROPERTY(move_animation);
		std::optional<Animation> PROPERTY(attack_animation);
		std::vector<Attachment> PROPERTY(attachments);

		bool _set_animation(std::string_view name, std::string_view file, fixed_point_t scroll_time);

	protected:
		Actor();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Actor(Actor&&) = default;
		virtual ~Actor() = default;

		OV_DETAIL_GET_TYPE
	};

	/* arrows.gfx */
	class ArrowType final : public Object {
		friend std::unique_ptr<ArrowType> std::make_unique<ArrowType>();

	private:
		//Named<> already handles the name property
		fixed_point_t PROPERTY(size, 5);
		//texture_file is unused, body_texture_file determines the appearance of the arrow
		std::string PROPERTY(texture_file); //unused
		std::string PROPERTY(body_texture_file);
		//colours dont appear to be used
		//TODO: Verify these property names for color and colortwo are correct
		colour_t PROPERTY(back_colour);
		colour_t PROPERTY(progress_colour);

		fixed_point_t PROPERTY(end_at, 1); //how should float be repd? >> fixed_point handles it
		fixed_point_t PROPERTY(height, 1);
		uint64_t PROPERTY(arrow_type, 0); //TODO: what does this do?
		fixed_point_t PROPERTY(heading, 1); //also float

		std::string PROPERTY(effect_file);

	protected:
		ArrowType();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		ArrowType(ArrowType&&) = default;
		virtual ~ArrowType() = default;

		OV_DETAIL_GET_TYPE
	};

	/* battlearrow.gfx */
	// BattleArrow and MapInfo are for the battle planner
	class BattleArrow final : public Object {
		friend std::unique_ptr<BattleArrow> std::make_unique<BattleArrow>();

	private:
		std::string PROPERTY(texture_arrow_body);
		std::string PROPERTY(texture_arrow_head);

		fixed_point_t PROPERTY(start, 1); //labelled 'body start width' in file
		fixed_point_t PROPERTY(stop, 1);  //labelled 'body end width' in file

		fvec2_t PROPERTY(dims, { 1, 1 }); //x,y labelled 'arrow length','arrow height' in file
		std::string PROPERTY(font);
		fixed_point_t PROPERTY(scale, 1);
		bool PROPERTY(no_fade, false);
		fixed_point_t PROPERTY(texture_loop);

	protected:
		BattleArrow();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		BattleArrow(BattleArrow&&) = default;
		virtual ~BattleArrow() = default;

		OV_DETAIL_GET_TYPE
	};

	class MapInfo final : public Object {
		friend std::unique_ptr<MapInfo> std::make_unique<MapInfo>();

	private:
		std::string PROPERTY(texture_file);
		fixed_point_t PROPERTY(scale, 1);

	protected:
		MapInfo();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		MapInfo(MapInfo&&) = default;
		virtual ~MapInfo() = default;

		OV_DETAIL_GET_TYPE
	};

	/* mapitems.gfx */
	class Projection final : public Object {
		friend std::unique_ptr<Projection> std::make_unique<Projection>();

	private:
		std::string PROPERTY(texture_file);
		//TODO: pulseSpeed, fadeout be ints or fixed points? assume fixed_point_t to start
		fixed_point_t PROPERTY(size, 1);
		fixed_point_t PROPERTY(spin, 1);
		bool PROPERTY(pulsating, false);
		fixed_point_t PROPERTY(pulse_lowest, 1);
		fixed_point_t PROPERTY(pulse_speed, 1);
		bool PROPERTY(additative, false);
		fixed_point_t PROPERTY(expanding, 1);
		fixed_point_t PROPERTY(duration, 0); //0 means it stays indefinitely (also the default value)
		fixed_point_t PROPERTY(fadeout, 0); //appears to have no effect

	protected:
		Projection();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Projection(Projection&&) = default;
		virtual ~Projection() = default;

		OV_DETAIL_GET_TYPE
	};

	using frame_t = int32_t; /* Keep this as int32_t to simplify interfacing with Godot */

	class Billboard final : public Object {
		friend std::unique_ptr<Billboard> std::make_unique<Billboard>();

	private:
		std::string PROPERTY(texture_file);
		fixed_point_t PROPERTY(scale, 1);
		frame_t PROPERTY(no_of_frames, 1);

	protected:
		Billboard();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Billboard(Billboard&&) = default;
		virtual ~Billboard() = default;

		OV_DETAIL_GET_TYPE
	};

	class ProgressBar3d final : public Object {
		friend std::unique_ptr<ProgressBar3d> std::make_unique<ProgressBar3d>();

	private:
		colour_t PROPERTY(back_colour);
		colour_t PROPERTY(progress_colour);
		ivec2_t PROPERTY(size);
		std::string PROPERTY(effect_file);

	protected:
		ProgressBar3d();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		ProgressBar3d(ProgressBar3d&&) = default;
		virtual ~ProgressBar3d() = default;

		OV_DETAIL_GET_TYPE
	};

	/* Core.gfx */
	class AnimatedMapText final : public Object {
		friend std::unique_ptr<AnimatedMapText> std::make_unique<AnimatedMapText>();

		using enum text_format_t;

	private:
		//textblock
		std::string PROPERTY(text);
		colour_t PROPERTY(colour);
		std::string PROPERTY(font);

		fvec2_t PROPERTY(text_position);
		fvec2_t PROPERTY(size);

		text_format_t PROPERTY(format, text_format_t::left);
		//end textblock

		fixed_point_t PROPERTY(speed, 1);
		fixed_point_t PROPERTY(scale, 1);
		fvec3_t PROPERTY(position);

	protected:
		AnimatedMapText();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		AnimatedMapText(AnimatedMapText&&) = default;
		virtual ~AnimatedMapText() = default;

		OV_DETAIL_GET_TYPE
	};

}