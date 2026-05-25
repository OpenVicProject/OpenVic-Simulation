#include "Deployment.hpp"

#include "openvic-simulation/military/Leader.hpp"

using namespace OpenVic;

UnitDeployment<unit_branch_t::LAND>::UnitDeployment(
	std::string_view new_name, RegimentType const& new_type, ProvinceDefinition const* new_home
) : name { new_name }, type { new_type }, home { new_home } {}

UnitDeployment<unit_branch_t::NAVAL>::UnitDeployment(std::string_view new_name, ShipType const& new_type)
	: name { new_name }, type { new_type } {}

Deployment::Deployment(
	std::string_view new_path, memory::vector<ArmyDeployment>&& new_armies, memory::vector<NavyDeployment>&& new_navies,
	memory::vector<LeaderBase>&& new_leaders
) : HasIdentifier { new_path }, armies { std::move(new_armies) }, navies { std::move(new_navies) },
	leaders { std::move(new_leaders) } {}
