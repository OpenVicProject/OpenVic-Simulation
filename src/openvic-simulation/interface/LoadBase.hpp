#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	template<typename... Context>
	class LoadBase {
	protected:
		LoadBase() = default;

		virtual bool _fill_key_map(NodeTools::key_map_t&, Context...) = 0;

	public:
		LoadBase(LoadBase&&) = default;
		virtual ~LoadBase() = default;

		bool load(ast::NodeCPtr node, Context... context) {
			NodeTools::key_map_t key_map;
			bool ret = _fill_key_map(key_map, context...);
			ret &= NodeTools::expect_dictionary_key_map(std::move(key_map))(node);
			return ret;
		}

		template<std::derived_from<LoadBase<Context...>> T, std::derived_from<T> U>
		static NodeTools::node_callback_t _expect_instance(
			NodeTools::callback_t<std::unique_ptr<T>&&> callback, Context... context
		) {
			return [callback, &context...](ast::NodeCPtr node) -> bool {
				std::unique_ptr<T> instance { std::make_unique<U>() };
				bool ret = instance->load(node, context...);
				ret &= callback(std::move(instance));
				return ret;
			};
		}

		OV_DETAIL_GET_TYPE_BASE_CLASS(LoadBase)
	};

	template<typename... Context>
	class Named : public LoadBase<Context...> {
		std::string PROPERTY(name);

	protected:
		Named() = default;

		virtual bool _fill_key_map(NodeTools::key_map_t& key_map, Context...) override {
			using namespace OpenVic::NodeTools;
			return add_key_map_entries(key_map, "name", ONE_EXACTLY, expect_string(assign_variable_callback_string(name)));
		}

		void _set_name(std::string_view new_name) {
			if (!name.empty()) {
				Logger::warning("Overriding scene name ", name, " with ", new_name);
			}
			name = new_name;
		}

	public:
		Named(Named&&) = default;
		virtual ~Named() = default;

		OV_DETAIL_GET_TYPE
	};

	template<typename Value, typename... Context>
	requires std::derived_from<Value, Named<Context...>>
	struct RegistryValueInfoNamed {
		using internal_value_type = Value;
		using external_value_type = Value;

		static constexpr std::string_view get_identifier(internal_value_type const& item) {
			return item.get_name();
		}
		static constexpr external_value_type& get_external_value(internal_value_type& item) {
			return item;
		}
		static constexpr external_value_type const& get_external_value(internal_value_type const& item) {
			return item;
		}
	};

	template<typename Value, typename... Context>
	requires std::derived_from<Value, Named<Context...>>
	using NamedRegistry = ValueRegistry<RegistryValueInfoNamed<Value, Context...>>;

	template<typename Value, typename... Context>
	requires std::derived_from<Value, Named<Context...>>
	using NamedInstanceRegistry = InstanceRegistry<RegistryValueInfoNamed<Value, Context...>>;
}
