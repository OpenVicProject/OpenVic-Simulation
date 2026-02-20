#include "WarHistory.hpp"

using namespace OpenVic;

WarHistory::WarHistory(
	std::string_view new_war_name,
	memory::vector<war_participant_t>&& new_attackers,
	memory::vector<war_participant_t>&& new_defenders,
	memory::vector<added_wargoal_t>&& new_wargoals
) : war_name { new_war_name }, attackers { std::move(new_attackers) }, defenders { std::move(new_defenders) },
	wargoals { std::move(new_wargoals) } {}