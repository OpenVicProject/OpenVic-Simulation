#pragma once

#include "Building.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct Good;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 */
	struct Province : HasIdentifier, HasColour {
		friend struct Map;

		using life_rating_t = int8_t;

	private:
		const index_t index;
		Region* region = nullptr;
		bool water = false;
		life_rating_t life_rating = 0;
		IdentifierRegistry<Building> buildings;
		// TODO - change this into a factory-like structure
		Good const* rgo = nullptr;

		Province(index_t new_index, std::string const& new_identifier, colour_t new_colour);

	public:
		Province(Province&&) = default;

		index_t get_index() const;
		Region* get_region() const;
		bool is_water() const;
		life_rating_t get_life_rating() const;
		return_t add_building(BuildingType const& type);
		void lock_buildings();
		void reset_buildings();
		Building const* get_building_by_identifier(std::string const& identifier) const;
		std::vector<Building> const& get_buildings() const;
		return_t expand_building(std::string const& building_type_identifier);
		Good const* get_rgo() const;
		std::string to_string() const;

		void update_state(Date const& today);
		void tick(Date const& today);
	};
}
