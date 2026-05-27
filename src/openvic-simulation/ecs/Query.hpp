#pragma once

#include <algorithm>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"

namespace OpenVic::ecs {

	// Builder for an archetype-matching query. Use `with<C...>()` to require components and
	// `exclude<C...>()` to forbid them. Call `build()` once before passing to a `for_each`
	// overload — `build()` sorts and deduplicates both lists so the World can compare them
	// against canonical sorted archetype signatures with a two-pointer scan, and so they
	// hash stably for the query cache.
	//
	// `with()` and `exclude()` may be called multiple times; lists accumulate. After `build()`
	// the same Query may be reused as long as no further `with`/`exclude` calls are made.
	struct Query {
		std::vector<component_type_id_t> require_ids;
		std::vector<component_type_id_t> exclude_ids;

		template<typename... Cs>
		Query& with() {
			(require_ids.push_back(component_type_id_of<Cs>()), ...);
			return *this;
		}

		template<typename... Cs>
		Query& exclude() {
			(exclude_ids.push_back(component_type_id_of<Cs>()), ...);
			return *this;
		}

		Query& build() {
			std::sort(require_ids.begin(), require_ids.end());
			require_ids.erase(std::unique(require_ids.begin(), require_ids.end()), require_ids.end());
			std::sort(exclude_ids.begin(), exclude_ids.end());
			exclude_ids.erase(std::unique(exclude_ids.begin(), exclude_ids.end()), exclude_ids.end());
			return *this;
		}

		bool operator==(Query const& other) const {
			return require_ids == other.require_ids && exclude_ids == other.exclude_ids;
		}
	};
}
