#pragma once

#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

#if defined(__APPLE__)
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Reform.hpp"
#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"
#endif

namespace OpenVic {
	struct BuildingType;
	struct GoodInstanceManager;
	struct Invention;
	struct PopsDefines;
	struct PopType;
	struct Reform;
	struct Technology;

	struct SharedCountryValuesDeps {
		GoodInstanceManager const& good_instance_manager;
		PopsDefines const& pop_defines;
		TypedSpan<building_type_index_t, const BuildingType> building_types;
		TypedSpan<invention_index_t, const Invention> inventions;
		TypedSpan<pop_type_index_t, const PopType> pop_types;
		TypedSpan<reform_index_t, const Reform> reforms;
		TypedSpan<regiment_type_index_t, const RegimentType> regiment_types;
		TypedSpan<technology_index_t, const Technology> technologies;
	};
}