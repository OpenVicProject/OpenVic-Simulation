#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

namespace OpenVic::dataloader {
	template<typename MapT>
	struct MapCallbackArguments {
		ovdl::v2script::Parser const* parser;
		ovdl::v2script::ast::Value const* node;
		MapT& map;
	};

	template<typename MapT>
	struct BaseMapCallback {
		static bool has(MapCallbackArguments<MapT> args) {
			return false;
		}
	};

	template<typename MapT>
	struct MapCallback;
}
