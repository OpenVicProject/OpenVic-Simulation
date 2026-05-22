#pragma once

#include <functional>
#include <string_view>

#include "openvic-simulation/military/UnitInstance.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include "openvic-simulation/military/UnitBranchedGetterMacro.hpp" //below other imports that undef the macros

namespace OpenVic {
	struct CountryInstance;
	struct LeaderInstance;
	struct MapInstance;
	struct ProvinceInstance;

	struct UnitInstanceGroup {
	private:
		memory::string PROPERTY(name);
		memory::vector<std::reference_wrapper<UnitInstance>> SPAN_PROPERTY(units);
		LeaderInstance* PROPERTY_PTR(leader, nullptr);
		std::reference_wrapper<ProvinceInstance> PROPERTY(location);
		std::reference_wrapper<CountryInstance> PROPERTY(country);

		fixed_point_t PROPERTY(total_organisation);
		fixed_point_t PROPERTY(total_max_organisation);
		fixed_point_t PROPERTY(total_strength);
		fixed_point_t PROPERTY(total_max_strength);

		// Movement attributes
		// Ordered list of provinces making up the path the unit is trying to move along,
		// the front province should always be adjacent to the unit's current location.
		memory::vector<std::reference_wrapper<ProvinceInstance>> SPAN_PROPERTY(path);
		// Measured in distance travelled, increases each day by unit speed (after modifiers have been applied) until
		// it reaches the required distance/movement cost to move to the next province in the path.
		fixed_point_t PROPERTY(movement_progress);

	protected:
		UnitInstanceGroup(
			unique_id_t new_unique_id,
			unit_branch_t new_branch,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);

		void update_gamestate();
		void tick();

	public:
		const unique_id_t unique_id;
		const unit_branch_t branch;

		UnitInstanceGroup(UnitInstanceGroup&&) = default;
		UnitInstanceGroup(UnitInstanceGroup const&) = delete;

		size_t get_unit_count() const;
		bool empty() const;
		size_t get_unit_category_count(UnitType::unit_category_t unit_category) const;
		UnitType const* get_display_unit_type() const;

		bool add_unit(UnitInstance& unit);
		bool remove_unit(UnitInstance const& unit);

		void set_name(std::string_view new_name);
		bool set_location(ProvinceInstance& new_location);
		bool set_country(CountryInstance& new_country);
		bool set_leader(LeaderInstance* new_leader);

		fixed_point_t get_organisation_proportion() const;
		fixed_point_t get_strength_proportion() const;

		fixed_point_t get_average_organisation() const;
		fixed_point_t get_average_max_organisation() const;

		bool is_moving() const;
		// The adjacent province that the unit will arrive in next, not necessarily the final destination of its current path
		ProvinceInstance const* get_movement_destination_province() const;
		Date get_movement_arrival_date() const;

		bool is_in_combat() const;

		constexpr bool operator==(UnitInstanceGroup const& rhs) const {
			return unique_id == rhs.unique_id;
		}
	};

	template<>
	struct UnitInstanceGroupBranched<unit_branch_t::LAND> : UnitInstanceGroup {
		using dig_in_level_t = uint8_t;

	private:
		NavyInstance* PROPERTY_PTR(transport_navy, nullptr);
		dig_in_level_t PROPERTY(dig_in_level, 0);

		bool PROPERTY_RW_CUSTOM_NAME(exiled, is_exiled, set_is_exiled, false);

	public:
		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		// TODO - do these work fine when units is empty?
		std::span<const std::reference_wrapper<RegimentInstance>> get_regiment_instances() {
			return { reinterpret_cast<std::reference_wrapper<RegimentInstance> const*>(get_units().data()), get_units().size() };
		}
		std::span<const std::reference_wrapper<const RegimentInstance>> get_regiment_instances() const {
			return { reinterpret_cast<std::reference_wrapper<const RegimentInstance> const*>(get_units().data()), get_units().size() };
		}
	};

	template<>
	struct UnitInstanceGroupBranched<unit_branch_t::NAVAL> : UnitInstanceGroup {
	private:
		memory::vector<std::reference_wrapper<ArmyInstance>> SPAN_PROPERTY(carried_armies);

	public:
		UnitInstanceGroupBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			CountryInstance& new_country,
			ProvinceInstance& new_location
		);
		UnitInstanceGroupBranched(UnitInstanceGroupBranched&&) = default;

		void update_gamestate();
		void tick();

		std::span<const std::reference_wrapper<ShipInstance>> get_ship_instances() {
			return { reinterpret_cast<std::reference_wrapper<ShipInstance> const*>(get_units().data()), get_units().size() };
		}
		std::span<const std::reference_wrapper<const ShipInstance>> get_ship_instances() const {
			return { reinterpret_cast<std::reference_wrapper<const ShipInstance> const*>(get_units().data()), get_units().size() };
		}

		fixed_point_t get_total_consumed_supply() const;
	};
}
