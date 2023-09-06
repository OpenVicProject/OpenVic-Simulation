#pragma once

#include "openvic/map/Building.hpp"
#include "openvic/pop/Pop.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct Good;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 */
	struct Province : HasIdentifierAndColour {
		friend struct Map;

		using index_t = uint16_t;
		using life_rating_t = int8_t;

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = (1 << (8 * sizeof(index_t))) - 1;

	private:
		const index_t index;
		Region* region = nullptr;
		bool water = false;
		life_rating_t life_rating = 0;
		IdentifierRegistry<Building> buildings;
		// TODO - change this into a factory-like structure
		Good const* rgo = nullptr;

		std::vector<Pop> pops;
		Pop::pop_size_t total_population;
		distribution_t pop_types, cultures, religions;

		Province(const std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		Province(Province&&) = default;

		index_t get_index() const;
		Region* get_region() const;
		bool is_water() const;
		life_rating_t get_life_rating() const;
		return_t add_building(Building&& building);
		void lock_buildings();
		void reset_buildings();
		Building const* get_building_by_identifier(const std::string_view identifier) const;
		size_t get_building_count() const;
		std::vector<Building> const& get_buildings() const;
		return_t expand_building(const std::string_view building_type_identifier);
		Good const* get_rgo() const;
		std::string to_string() const;

		return_t load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root);
		return_t add_pop(Pop&& pop);
		void clear_pops();
		size_t get_pop_count() const;
		std::vector<Pop> const& get_pops() const;
		Pop::pop_size_t get_total_population() const;
		distribution_t const& get_pop_type_distribution() const;
		distribution_t const& get_culture_distribution() const;
		distribution_t const& get_religion_distribution() const;
		void update_pops();

		void update_state(Date const& today);
		void tick(Date const& today);
	};
}
