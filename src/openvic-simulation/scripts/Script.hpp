#pragma once

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic {
	// TODO - is template needed if context is always DefinitionManager const&?
	template<typename... _Context>
	struct Script {
	private:
		memory::vector<ovdl::v2script::ast::Node const*> _root_nodes;

	protected:
		virtual bool _parse_script(std::span<ovdl::v2script::ast::Node const* const> nodes, _Context... context) = 0;

	public:
		constexpr Script() {};
		Script(Script&&) = default;

		constexpr bool has_defines_node() const {
			return !_root_nodes.empty();
		}

		constexpr NodeTools::NodeCallback auto expect_script() {
			return NodeTools::vector_callback(_root_nodes);
		}

		bool parse_script(bool can_be_null, _Context... context) {
			if (_root_nodes.empty()) {
				if (!can_be_null) {
					spdlog::error_s("Missing script node!");
				}
				return can_be_null;
			}

			if (_root_nodes.size() > 1) {
				spdlog::warn_s("Multiple root nodes");
			}

			const bool ret = _parse_script(_root_nodes, context...);
			_root_nodes.clear();
			_root_nodes.shrink_to_fit();
			return ret;
		}
	};
}
