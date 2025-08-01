#pragma once

#include <cstdint>
#include <string_view>

namespace OpenVic {
	enum struct unit_branch_t : uint8_t { INVALID_BRANCH, LAND, NAVAL };

	template<unit_branch_t>
	struct UnitTypeBranched;

	using RegimentType = UnitTypeBranched<unit_branch_t::LAND>;
	// Each value is a subset of its predecessor, so smaller values contain larger values
	// The exact values here must be preserved to allow easy comparison against Pop::culture_status_t
	enum struct regiment_allowed_cultures_t : uint8_t { ALL_CULTURES, ACCEPTED_CULTURES, PRIMARY_CULTURE, NO_CULTURES };

	using ShipType = UnitTypeBranched<unit_branch_t::NAVAL>;

	template<unit_branch_t>
	struct UnitInstanceBranched;
	using RegimentInstance = UnitInstanceBranched<unit_branch_t::LAND>;
	using ShipInstance = UnitInstanceBranched<unit_branch_t::NAVAL>;

	template<unit_branch_t>
	struct UnitInstanceGroupBranched;
	using ArmyInstance = UnitInstanceGroupBranched<unit_branch_t::LAND>;
	using NavyInstance = UnitInstanceGroupBranched<unit_branch_t::NAVAL>;

	
	static constexpr std::string_view get_branch_name(unit_branch_t branch) {
		using enum unit_branch_t;

		switch (branch) {
		case LAND:
			return "land";
		case NAVAL:
			return "naval";
		default:
			return "INVALID BRANCH";
		}
	}
	static constexpr std::string_view get_branched_unit_name(unit_branch_t branch) {
		using enum unit_branch_t;

		switch (branch) {
		case LAND:
			return "regiment";
		case NAVAL:
			return "ship";
		default:
			return "INVALID UNIT BRANCH";
		}
	}
	static constexpr std::string_view get_branched_unit_group_name(unit_branch_t branch) {
		using enum unit_branch_t;

		switch (branch) {
		case LAND:
			return "army";
		case NAVAL:
			return "navy";
		default:
			return "INVALID UNIT GROUP BRANCH";
		}
	}
	static constexpr std::string_view get_branched_leader_name(unit_branch_t branch) {
		using enum unit_branch_t;

		switch (branch) {
		case LAND:
			return "general";
		case NAVAL:
			return "admiral";
		default:
			return "INVALID LEADER BRANCH";
		}
	}
}