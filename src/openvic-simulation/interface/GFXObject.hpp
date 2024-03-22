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
		class Attachment : public Named<> {
			friend class LoadBase;

		private:
			std::string PROPERTY(node);
			int32_t PROPERTY(attach_id);

		protected:
			Attachment();

			bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

		public:
			Attachment(Attachment&&) = default;
			virtual ~Attachment() = default;
		};

		class Animation : public Named<> {
			friend class LoadBase;

			std::string PROPERTY(file);
			fixed_point_t PROPERTY(default_time);

		protected:
			Animation();

			bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

		public:
			Animation(Animation&&) = default;
			virtual ~Animation() = default;
		};

	private:
		std::string PROPERTY(model_file);
		std::string PROPERTY(idle_animation_file);
		std::string PROPERTY(move_animation_file);
		std::string PROPERTY(attack_animation_file);
		fixed_point_t PROPERTY(scale);

		NamedRegistry<Attachment> IDENTIFIER_REGISTRY(attachment);
		NamedRegistry<Animation> IDENTIFIER_REGISTRY(animation);

	protected:
		Actor();

		bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) override;

	public:
		Actor(Actor&&) = default;
		virtual ~Actor() = default;

		OV_DETAIL_GET_TYPE
	};
}
