#pragma once

#include <string_view>

#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	struct UnitInstance {
	private:
		const unique_id_t PROPERTY(unique_id);
		memory::string PROPERTY(name);
		fixed_point_t PROPERTY(organisation);
		fixed_point_t PROPERTY(max_organisation);
		fixed_point_t PROPERTY(strength);

	protected:
		UnitInstance(
			unique_id_t new_unique_id,
			std::string_view new_name,
			UnitType const& new_unit_type
		);

	public:
		UnitType const& unit_type;

		UnitInstance(UnitInstance&&) = default;

		inline constexpr fixed_point_t get_max_strength() const {
			return unit_type.get_max_strength();
		}

		inline constexpr unit_branch_t get_branch() const {
			return unit_type.get_branch();
		}

		void set_name(std::string_view new_name);
	};

	struct Pop;

	template<>
	struct UnitInstanceBranched<unit_branch_t::LAND> : UnitInstance {
		friend struct UnitInstanceManager;

	private:
		Pop* PROPERTY_PTR(pop);
		bool PROPERTY_CUSTOM_PREFIX(mobilised, is);

		UnitInstanceBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			RegimentType const& new_regiment_type,
			Pop* new_pop,
			bool new_mobilised
		);

	public:
		UnitInstanceBranched(UnitInstanceBranched&&) = default;

		constexpr RegimentType const& get_regiment_type() const {
			return static_cast<RegimentType const&>(unit_type);
		}
	};

	template<>
	struct UnitInstanceBranched<unit_branch_t::NAVAL> : UnitInstance {
		friend struct UnitInstanceManager;

	private:
		UnitInstanceBranched(
			unique_id_t new_unique_id,
			std::string_view new_name,
			ShipType const& new_ship_type
		);

	public:
		UnitInstanceBranched(UnitInstanceBranched&&) = default;

		constexpr ShipType const& get_ship_type() const {
			return static_cast<ShipType const&>(unit_type);
		}
	};
}
