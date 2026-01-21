
#pragma once

#include "openvic-simulation/core/container/CaseContainer.hpp"
#include "openvic-simulation/core/memory/OrderedSet.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/template/StringMapCasing.hpp"

namespace OpenVic::memory {
	/* Template for set with string elements, supporting search by string_view without creating an intermediate string. */
	template<string_map_case Case>
	using template_string_set_t = template_case_container_t<ordered_set, Case, memory::string>;

	using string_set_t = template_string_set_t<StringMapCaseSensitive>;
	using case_insensitive_string_set_t = template_string_set_t<StringMapCaseInsensitive>;
}
