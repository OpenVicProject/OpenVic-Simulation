#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-dataloader/v2script/Parser.hpp"

namespace OpenVic {
	// TODO - is template needed if context is always DefinitionManager const&?
	template<typename... _Context>
	struct Script {
	private:
		ast::NodeCPtr _root;
		ovdl::v2script::Parser const* _parser;

	protected:
		virtual bool _parse_script(ast::NodeCPtr root, ovdl::v2script::Parser const& parser, _Context... context) = 0;

	public:
		Script() : _root { nullptr }, _parser { nullptr } {}
		Script(Script&&) = default;

		constexpr bool has_defines_node() const {
			return _root != nullptr;
		}

		constexpr NodeTools::NodeCallback auto expect_script() {
			// TODO: assign the node too?
			return NodeTools::assign_variable_callback(_parser);
		}

		bool parse_script(bool can_be_null, _Context... context) {
			if (_root == nullptr || _parser == nullptr) {
				if (!can_be_null) {
					Logger::error("Null/missing script node!");
				}
				return can_be_null;
			}
			const bool ret = _parse_script(_root, _parser, context...);
			_root = nullptr;
			_parser = nullptr;
			return ret;
		}
	};
}
