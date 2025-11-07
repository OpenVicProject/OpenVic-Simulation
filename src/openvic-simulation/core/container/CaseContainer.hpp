#pragma once

#include "openvic-simulation/core/template/Concepts.hpp"

namespace OpenVic {
	/* Intermediate struct that "remembers" Case, instead of just decomposing it into its hash and equal components,
	 * needed so that templates can deduce the Case with which a type was defined. */
	template<template<typename...> typename Container, string_map_case Case, typename... Args>
	struct template_case_container_t : Container<Args..., typename Case::hash, typename Case::equal> {
		using container_t = Container<Args..., typename Case::hash, typename Case::equal>;
		using container_t::container_t;

		using case_t = Case;
	};
}
