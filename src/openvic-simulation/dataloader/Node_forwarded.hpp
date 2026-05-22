#pragma once

namespace dryad {
	template <typename T> class node;
}

namespace ovdl::v2script::ast {
	enum class NodeKind;
	using Node = dryad::node<ovdl::v2script::ast::NodeKind>;
}