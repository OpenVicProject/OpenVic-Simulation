#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/definition/dataloader/TraverseResult.hpp"

namespace OpenVic::dataloader {
	template<typename MapT>
	struct MapCallbackArguments {
		TraverseResult& traverse;
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
