#pragma once

#include <functional>
#include <variant>

namespace OpenVic {
	struct PartyPolicyGroup;
	struct ReformGroup;
	using issue_group_t =
		std::variant<std::monostate, std::reference_wrapper<const PartyPolicyGroup>, std::reference_wrapper<const ReformGroup>>;
}
