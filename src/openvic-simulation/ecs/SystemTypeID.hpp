#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"

namespace OpenVic::ecs {
	using system_type_id_t = uint64_t;

	// Primary template intentionally undefined. Every system used with World must specialise
	// this trait — the typical path is the ECS_SYSTEM macro defined below. Failure to register
	// becomes a clear compile error: "incomplete type SystemName<X>".
	template<typename S>
	struct SystemName;

	template<typename S>
	constexpr system_type_id_t system_type_id_of() {
		return fnv1a_64(SystemName<S>::value);
	}
}

// Specialise OpenVic::ecs::SystemName<Type> with a stable string literal that becomes the
// system's identity across all builds. Must be invoked at namespace scope (outside any
// other namespace). The literal must be globally unique within the simulation; renames are
// breaking changes to anything that persists system IDs (saves, replays, multiplayer
// schedule-hash handshake).
//
// The macro stringifies its argument via #Type, so the literal matches the qualified name
// passed in. Example:
//   namespace OpenVic { struct UnitGroupTotalsSystem { ... }; }
//   ECS_SYSTEM(OpenVic::UnitGroupTotalsSystem)
// expands to value = "OpenVic::UnitGroupTotalsSystem".
#define ECS_SYSTEM(Type) \
	namespace OpenVic::ecs { \
		template<> \
		struct SystemName<Type> { \
			static constexpr std::string_view value = #Type; \
		}; \
	}
