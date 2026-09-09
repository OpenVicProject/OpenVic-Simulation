#pragma once

#include <cstdint>
#include <string_view>

namespace OpenVic::ecs {
	using component_type_id_t = uint64_t;

	// FNV-1a 64-bit. Pure constexpr — the same input string yields the same hash on every
	// compiler and platform, so component IDs are byte-identical across builds. This is the
	// foundation OpenVic relies on for cross-platform deterministic protocols (multiplayer,
	// save sharing, replay logs).
	constexpr component_type_id_t fnv1a_64(std::string_view s) {
		constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;
		constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
		uint64_t h = FNV_OFFSET;
		for (char c : s) {
			h ^= static_cast<uint8_t>(c);
			h *= FNV_PRIME;
		}
		return h;
	}

	// Primary template intentionally undefined. Every component used with World must specialise
	// this trait — the typical path is the ECS_COMPONENT macro defined below. Failure to register
	// becomes a clear compile error: "incomplete type ComponentName<X>".
	template<typename C>
	struct ComponentName;

	template<typename C>
	constexpr component_type_id_t component_type_id_of() {
		return fnv1a_64(ComponentName<C>::value);
	}
}

// Specialise OpenVic::ecs::ComponentName<Type> with a stable string literal that becomes the
// component's identity across all builds. Must be invoked at namespace scope (outside any other
// namespace). The literal must be globally unique within the simulation; renames are breaking
// changes to anything that persists component IDs (saves, replays, network protocol).
//
// Example:
//   struct LeaderTemplate { ... };
//   ECS_COMPONENT(OpenVic::LeaderTemplate, "OpenVic::LeaderTemplate")
#define ECS_COMPONENT(Type, NameLiteral) \
	namespace OpenVic::ecs { \
		template<> \
		struct ComponentName<Type> { \
			static constexpr std::string_view value = NameLiteral; \
		}; \
	}
