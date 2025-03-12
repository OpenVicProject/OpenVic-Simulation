#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	// TODO - is template needed if context is always DefinitionManager const&?
	template<typename... _Context>
	struct Script {
	private:
		ast::NodeCPtr _root = nullptr;

	protected:
		virtual bool _parse_script(ast::NodeCPtr root, _Context... context) = 0;

	public:
		Script() {}
		Script(Script&&) = default;

		constexpr bool has_defines_node() const {
			return _root != nullptr;
		}

		constexpr NodeTools::NodeCallback auto expect_script() {
			return NodeTools::assign_variable_callback(_root);
		}

		bool parse_script(bool can_be_null, _Context... context) {
			if (_root == nullptr) {
				if (!can_be_null) {
					Logger::error("Null/missing script node!");
				}
				return can_be_null;
			}
			const bool ret = _parse_script(_root, context...);
			_root = nullptr;
			return ret;
		}
	};
}
