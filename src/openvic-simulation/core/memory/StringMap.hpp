#pragma once

#include "openvic-simulation/core/container/CaseContainer.hpp"
#include "openvic-simulation/core/container/OrderedMap.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/template/StringMapCasing.hpp"

namespace OpenVic::memory {
	/* Template for map with string keys, supporting search by string_view without creating an intermediate string. */
	template<typename T, string_map_case Case>
	using template_string_map_t = template_case_container_t<ordered_map, Case, memory::string, T>;

	template<typename T>
	using string_map_t = template_string_map_t<T, StringMapCaseSensitive>;
	template<typename T>
	using case_insensitive_string_map_t = template_string_map_t<T, StringMapCaseInsensitive>;
}
