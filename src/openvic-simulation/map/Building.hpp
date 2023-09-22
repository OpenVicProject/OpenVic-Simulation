#pragma once

#include <string_view>
#include <vector>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

	struct BuildingManager;
	struct BuildingType;

	/* REQUIREMENTS:
	 * MAP-11, MAP-72, MAP-73
	 * MAP-12, MAP-75, MAP-76
	 * MAP-13, MAP-78, MAP-79
	 */
	struct Building : HasIdentifier {
		friend struct BuildingManager;

		using level_t = int8_t;

		enum class ExpansionState {
			CannotExpand,
			CanExpand,
			Preparing,
			Expanding
		};

	private:
		BuildingType const& type;
		level_t level = 0;
		ExpansionState expansion_state = ExpansionState::CannotExpand;
		Date start, end;
		float expansion_progress;

		Building(BuildingType const& new_type);

		bool _can_expand() const;

	public:
		Building(Building&&) = default;

		BuildingType const& get_type() const;
		level_t get_level() const;
		ExpansionState get_expansion_state() const;
		Date const& get_start_date() const;
		Date const& get_end_date() const;
		float get_expansion_progress() const;

		bool expand();
		void update_state(Date const& today);
		void tick(Date const& today);
	};

	struct BuildingType : HasIdentifier {
		friend struct BuildingManager;

	private:
		const Building::level_t max_level;
		const Timespan build_time;

		BuildingType(const std::string_view new_identifier, Building::level_t new_max_level, Timespan new_build_time);

	public:
		BuildingType(BuildingType&&) = default;

		Building::level_t get_max_level() const;
		Timespan get_build_time() const;
	};

	struct Province;

	struct BuildingManager {
	private:
		IdentifierRegistry<BuildingType> building_types;	// TODO: This needs a getter

	public:
		BuildingManager();

		bool add_building_type(const std::string_view identifier, Building::level_t max_level, Timespan build_time);
		IDENTIFIER_REGISTRY_ACCESSORS(BuildingType, building_type)
		bool generate_province_buildings(Province& province) const;
	};
}
