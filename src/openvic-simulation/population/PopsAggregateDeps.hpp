#pragma once

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"

namespace OpenVic {
	struct Ideology;
	struct PartyPolicy;
	struct Reform;

	struct PopsAggregateDeps {
		forwardable_span<const Ideology> ideologies;
		forwardable_span<const PartyPolicy> party_policies;
		forwardable_span<const Reform> reforms;
	};
}