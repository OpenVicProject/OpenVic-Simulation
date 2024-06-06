#pragma once

#include <filesystem>

#include <openvic-dataloader/csv/LineObject.hpp>

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	namespace fs = std::filesystem;

	struct GoodManager;
	struct ProvinceHistoryManager;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct Map {
#pragma pack(push, 1)
		/* Used to represent tightly packed 3-byte integer pixel information. */
		struct shape_pixel_t {
			ProvinceDefinition::index_t index;
			TerrainTypeMapping::index_t terrain;
		};
#pragma pack(pop)
	private:
		using colour_index_map_t = ordered_map<colour_t, ProvinceDefinition::index_t>;

		IdentifierRegistry<ProvinceDefinition> IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(province_definition, 1);
		IdentifierRegistry<ProvinceInstance> IDENTIFIER_REGISTRY_CUSTOM_INDEX_OFFSET(province_instance, 1);
		IdentifierRegistry<Region> IDENTIFIER_REGISTRY(region);
		IdentifierRegistry<Climate> IDENTIFIER_REGISTRY(climate);
		IdentifierRegistry<Continent> IDENTIFIER_REGISTRY(continent);
		ProvinceSet water_provinces;
		TerrainTypeManager PROPERTY_REF(terrain_type_manager);

		ivec2_t PROPERTY(dims);
		std::vector<shape_pixel_t> PROPERTY(province_shape_image);
		colour_index_map_t colour_index_map;

		ProvinceDefinition::index_t PROPERTY(max_provinces);
		ProvinceInstance* PROPERTY(selected_province); // is it right for this to be mutable? how about using an index instead?
		Pop::pop_size_t PROPERTY(highest_province_population);
		Pop::pop_size_t PROPERTY(total_map_population);

		ProvinceDefinition::index_t get_index_from_colour(colour_t colour) const;
		bool _generate_standard_province_adjacencies();

		StateManager PROPERTY_REF(state_manager);

		inline constexpr int32_t get_pixel_index_from_pos(ivec2_t pos) const {
			return pos.x + pos.y * dims.x;
		}

	public:
		Map();

		inline constexpr int32_t get_width() const { return dims.x; }
		inline constexpr int32_t get_height() const { return dims.y; }

		bool add_province_definition(std::string_view identifier, colour_t colour);

	private:
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(province_definition, 1);

	public:
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS_CUSTOM_INDEX_OFFSET(province_instance, 1);

		ProvinceInstance* get_province_instance_from_const(ProvinceDefinition const* province);
		ProvinceInstance const* get_province_instance_from_const(ProvinceDefinition const* province) const;

		ProvinceDefinition::distance_t calculate_distance_between(
			ProvinceDefinition const& from, ProvinceDefinition const& to
		) const;
		bool add_standard_adjacency(ProvinceDefinition& from, ProvinceDefinition& to) const;
		bool add_special_adjacency(
			ProvinceDefinition& from, ProvinceDefinition& to, ProvinceDefinition::adjacency_t::type_t type,
			ProvinceDefinition const* through, ProvinceDefinition::adjacency_t::data_t data
		) const;

		bool set_water_province(std::string_view identifier);
		bool set_water_province_list(std::vector<std::string_view> const& list);
		void lock_water_provinces();

		ProvinceDefinition::index_t get_province_index_at(ivec2_t pos) const;

	private:
		ProvinceDefinition* get_province_definition_at(ivec2_t pos);

		/* This provides a safe way to remove the const qualifier of a ProvinceDefinition const*, via a non-const Map.
		 * It uses a const_cast (the fastest/simplest solution), but this could also be done without it by looking up the
		 * ProvinceDefinition* using the ProvinceDefinition const*'s index. Requiring a non-const Map ensures that this
		 * function can only be used where the ProvinceDefinition* could already be accessed by other means, such as the
		 * index method, preventing misleading code, or in the worst case undefined behaviour. */
		constexpr ProvinceDefinition* remove_province_definition_const(ProvinceDefinition const* province) {
			return const_cast<ProvinceDefinition*>(province);
		}

	public:
		ProvinceDefinition const* get_province_definition_at(ivec2_t pos) const;
		bool set_max_provinces(ProvinceDefinition::index_t new_max_provinces);
		void set_selected_province(ProvinceDefinition::index_t index);
		ProvinceInstance* get_selected_province();
		ProvinceDefinition::index_t get_selected_province_index() const;

		bool add_region(std::string_view identifier, std::vector<ProvinceDefinition const*>&& provinces, colour_t colour);

		bool reset(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_provinces(
			ProvinceHistoryManager const& history_manager, Date date, IdeologyManager const& ideology_manager,
			IssueManager const& issue_manager, Country const& country
		);

		void update_gamestate(Date today);
		void tick(Date today);

		bool load_province_definitions(std::vector<ovdl::csv::LineObject> const& lines);
		/* Must be loaded after adjacencies so we know what provinces are coastal, and so can have a port */
		bool load_province_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root);
		static bool load_region_colours(ast::NodeCPtr root, std::vector<colour_t>& colours);
		bool load_region_file(ast::NodeCPtr root, std::vector<colour_t> const& colours);
		bool load_map_images(fs::path const& province_path, fs::path const& terrain_path, bool detailed_errors);
		bool generate_and_load_province_adjacencies(std::vector<ovdl::csv::LineObject> const& additional_adjacencies);
		bool load_climate_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
		bool load_continent_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
}
