#pragma once

#include <concepts>
#include <string>
#include <string_view>

#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	template<UnitType::branch_t Branch>
	struct UnitInstance {
		using _UnitType = UnitTypeBranched<Branch>;

	private:
		std::string PROPERTY(unit_name);
		_UnitType const& PROPERTY(unit_type);

		fixed_point_t PROPERTY_RW(organisation);
		fixed_point_t PROPERTY_RW(morale);
		fixed_point_t PROPERTY_RW(strength);

	protected:
		UnitInstance(std::string_view new_unit_name, _UnitType const& new_unit_type);

	public:
		UnitInstance(UnitInstance&&) = default;

		void set_unit_name(std::string_view new_unit_name);
	};

	struct Pop;

	template<UnitType::branch_t>
	struct UnitInstanceBranched;

	template<>
	struct UnitInstanceBranched<UnitType::branch_t::LAND> : UnitInstance<UnitType::branch_t::LAND> {
		friend struct UnitInstanceManager;

	private:
		Pop* PROPERTY(pop);
		bool PROPERTY_CUSTOM_PREFIX(mobilised, is);

		UnitInstanceBranched(
			std::string_view new_name, RegimentType const& new_regiment_type, Pop* new_pop, bool new_mobilised)
		;

	public:
		UnitInstanceBranched(UnitInstanceBranched&&) = default;
	};

	using RegimentInstance = UnitInstanceBranched<UnitType::branch_t::LAND>;

	template<>
	struct UnitInstanceBranched<UnitType::branch_t::NAVAL> : UnitInstance<UnitType::branch_t::NAVAL> {
		friend struct UnitInstanceManager;

	private:
		UnitInstanceBranched(std::string_view new_name, ShipType const& new_ship_type);

	public:
		UnitInstanceBranched(UnitInstanceBranched&&) = default;
	};

	using ShipInstance = UnitInstanceBranched<UnitType::branch_t::NAVAL>;
}
