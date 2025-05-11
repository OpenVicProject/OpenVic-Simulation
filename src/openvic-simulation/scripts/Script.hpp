#pragma once

#include <vector>

#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	// TODO - is template needed if context is always DefinitionManager const&?
	template<typename... _Context>
	struct Script {
	private:
		memory::vector<ast::NodeCPtr> _root_nodes;

	protected:
		virtual bool _parse_script(std::span<const ast::NodeCPtr> nodes, _Context... context) = 0;

	public:
		Script() = default;
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
					Logger::error("Missing script node!");
				}
				return can_be_null;
			}

			if (_root_nodes.size() > 1) {
				Logger::warning("Multiple root nodes");
			}

			const bool ret = _parse_script(_root_nodes, context...);
			_root_nodes.clear();
			_root_nodes.shrink_to_fit();
			return ret;
		}
	};
}
