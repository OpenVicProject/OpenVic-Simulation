#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	template<typename... _Context>
	class LoadBase {
	protected:
		LoadBase() = default;

		virtual bool _fill_key_map(NodeTools::key_map_t&, _Context...) = 0;

	public:
		LoadBase(LoadBase&&) = default;
		virtual ~LoadBase() = default;

		bool load(ast::NodeCPtr node, _Context... context) {
			NodeTools::key_map_t key_map;
			bool ret = _fill_key_map(key_map, context...);
			ret &= NodeTools::expect_dictionary_key_map(std::move(key_map))(node);
			return ret;
		}

		template<std::derived_from<LoadBase<_Context...>> T, std::derived_from<T> U>
		static NodeTools::node_callback_t _expect_instance(
			NodeTools::callback_t<std::unique_ptr<T>&&> callback, _Context... context
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

	template<typename... _Context>
	class Named : public LoadBase<_Context...> {
		std::string PROPERTY(name);

	protected:
		Named() = default;

		virtual bool _fill_key_map(NodeTools::key_map_t& key_map, _Context...) override {
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

	template<typename T, typename... _Context>
	requires std::derived_from<T, Named<_Context...>>
	struct _get_name {
		constexpr std::string_view operator()(T const* item) const {
			return item->get_name();
		}
	};

	template<typename T, typename... _Context>
	requires std::derived_from<T, Named<_Context...>>
	using NamedRegistry = ValueRegistry<T, _get_name<T, _Context...>>;

	template<typename T, typename... _Context>
	requires std::derived_from<T, Named<_Context...>>
	using NamedInstanceRegistry = InstanceRegistry<T, _get_name<T, _Context...>>;
}
