#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <plf_colony.h>

#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	struct ProvinceInstance;

	struct MovementInfo {
	private:
		std::vector<ProvinceInstance const*> PROPERTY(path);
		fixed_point_t PROPERTY(movement_progress);

	public:
		MovementInfo();
		// contains/calls pathfinding logic
		MovementInfo(ProvinceInstance const* starting_province, ProvinceInstance const* target_province);
	};

	template<UnitType::branch_t>
	struct LeaderBranched;

	struct CountryInstance;

	template<UnitType::branch_t>
	struct UnitInstanceGroupBranched;

	template<UnitType::branch_t Branch>
	struct UnitInstanceGroup {
		using _UnitInstance = UnitInstanceBranched<Branch>;
		using _Leader = LeaderBranched<Branch>;

	private:
		std::string PROPERTY(name);
		std::vector<_UnitInstance*> PROPERTY(units);
		_Leader* PROPERTY(leader);

		MovementInfo PROPERTY_REF(movement_info);

	protected:
		ProvinceInstance* PROPERTY_ACCESS(position, protected);
		CountryInstance* PROPERTY_ACCESS(country, protected);

		UnitInstanceGroup(
			std::string_view new_name,
			std::vector<_UnitInstance*>&& new_units,
			_Leader* new_leader,
			CountryInstance* new_country
		) : name { new_name },
			units { std::move(new_units) },
			leader { nullptr },
			position { nullptr },
			country { new_country } {
			set_leader(new_leader);
		}

	public:
		UnitInstanceGroup(UnitInstanceGroup&&) = default;
		UnitInstanceGroup(UnitInstanceGroup const&) = delete;

		void set_name(std::string_view new_name) {
			name = new_name;
		}

		size_t get_unit_count() const {
			return units.size();
		}

		bool empty() const {
			return units.empty();
		}

		size_t get_unit_category_count(UnitType::unit_category_t unit_category) const {
			return std::count_if(units.begin(), units.end(), [unit_category](_UnitInstance const* unit) {
				return unit->unit_type.get_unit_category() == unit_category;
			});
		}

		UnitType const* get_display_unit_type() const {
			if (units.empty()) {
				return nullptr;
			}

			fixed_point_map_t<UnitType const*> weighted_unit_types;

			for (_UnitInstance const* unit : units) {
				UnitType const& unit_type = unit->get_unit_type();
				weighted_unit_types[&unit_type] += unit_type.get_weighted_value();
			}

			return get_largest_item_tie_break(
				weighted_unit_types,
				[](UnitType const* lhs, UnitType const* rhs) -> bool {
					return lhs->get_weighted_value() < rhs->get_weighted_value();
				}
			)->first;
		}

		void set_position(ProvinceInstance* new_position) {
			if (position != new_position) {
				if (position != nullptr) {
					position->remove_unit_instance_group(*this);
				}

				position = new_position;

				if (position != nullptr) {
					position->add_unit_instance_group(*this);
				}
			}
		}

		bool set_leader(_Leader* new_leader) {
			bool ret = true;

			if (leader != new_leader) {
				if (leader != nullptr) {
					if (leader->unit_instance_group == this) {
						leader->unit_instance_group = nullptr;
					} else {
						Logger::error(
							"Mismatch between leader and unit instance group: group ", name, " has leader ",
							leader->get_name(), " but the leader has group ", leader->get_unit_instance_group() != nullptr
								? leader->get_unit_instance_group()->get_name() : "NULL"
						);
						ret = false;
					}
				}

				leader = new_leader;

				if (leader != nullptr) {
					if (leader->unit_instance_group != nullptr) {
						if (leader->unit_instance_group != this) {
							ret &= leader->unit_instance_group->set_leader(nullptr);
						} else {
							Logger::error("Leader ", leader->get_name(), " already leads group ", name, "!");
							ret = false;
						}
					}

					leader->unit_instance_group = static_cast<UnitInstanceGroupBranched<Branch>*>(this);
				}
			}

			return ret;
		}
	};

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::LAND> : UnitInstanceGroup<UnitType::branch_t::LAND> {
		friend struct UnitInstanceManager;

	private:
		UnitInstanceGroupBranched(
			std::string_view new_name,
			std::vector<RegimentInstance*>&& new_units,
			_Leader* new_leader,
			CountryInstance* new_country
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;
	};

	using ArmyInstance = UnitInstanceGroupBranched<UnitType::branch_t::LAND>;

	template<>
	struct UnitInstanceGroupBranched<UnitType::branch_t::NAVAL> : UnitInstanceGroup<UnitType::branch_t::NAVAL> {
		friend struct UnitInstanceManager;

	private:
		std::vector<ArmyInstance const*> PROPERTY(carried_armies);

		UnitInstanceGroupBranched(
			std::string_view new_name,
			std::vector<ShipInstance*>&& new_ships,
			_Leader* new_leader,
			CountryInstance* new_country
		);

	public:
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;
	};

	using NavyInstance = UnitInstanceGroupBranched<UnitType::branch_t::NAVAL>;

	struct RegimentDeployment;
	struct ShipDeployment;
	struct MapInstance;
	struct ArmyDeployment;
	struct NavyDeployment;
	struct Deployment;

	struct UnitInstanceManager {
	private:
		plf::colony<RegimentInstance> PROPERTY(regiments);
		plf::colony<ShipInstance> PROPERTY(ships);

		plf::colony<ArmyInstance> PROPERTY(armies);
		plf::colony<NavyInstance> PROPERTY(navies);

		bool generate_regiment(RegimentDeployment const& regiment_deployment, RegimentInstance*& regiment);
		bool generate_ship(ShipDeployment const& ship_deployment, ShipInstance*& ship);
		bool generate_army(MapInstance& map_instance, CountryInstance& country, ArmyDeployment const& army_deployment);
		bool generate_navy(MapInstance& map_instance, CountryInstance& country, NavyDeployment const& navy_deployment);

	public:
		bool generate_deployment(MapInstance& map_instance, CountryInstance& country, Deployment const* deployment);
	};
}
