#pragma once

#include "openvic-simulation/interface/LoadBase.hpp"

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

			Attachment(std::string_view new_actor_name, std::string_view new_attach_node, attach_id_t new_attach_id);

		public:
			Attachment(Attachment&&) = default;
		};

		class Animation {
			friend class Actor;

			std::string PROPERTY(file);
			fixed_point_t PROPERTY(scroll_time);

			Animation(std::string_view new_file, fixed_point_t new_scroll_time);

		public:
			Animation(Animation&&) = default;
		};

	private:
		fixed_point_t PROPERTY(scale);
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
}
