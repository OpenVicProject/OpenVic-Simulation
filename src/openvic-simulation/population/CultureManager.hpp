#pragma once

#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {
	struct CultureManager;
	struct CountryDefinition;
	struct CountryDefinitionManager;
	class Dataloader;

	struct CultureManager {
		using leader_count_t = uint32_t;

	private:
		IdentifierRegistry<GraphicalCultureType> IDENTIFIER_REGISTRY(graphical_culture_type);
		IdentifierRegistry<CultureGroup> IDENTIFIER_REGISTRY(culture_group);
		IdentifierRegistry<Culture> IDENTIFIER_REGISTRY(culture);

		GraphicalCultureType const* PROPERTY(default_graphical_culture_type);

		using general_admiral_picture_count_t = std::pair<leader_count_t, leader_count_t>;
		// Cultural type string maps to (general picture count, admiral picture count) pair
		string_map_t<general_admiral_picture_count_t> leader_picture_counts;

		bool _load_culture_group(
			CountryDefinitionManager const& country_definition_manager, size_t& total_expected_cultures,
			std::string_view culture_group_key, ovdl::v2script::ast::Node const* culture_group_node
		);
		bool _load_culture(
			CountryDefinitionManager const& country_definition_manager, CultureGroup const& culture_group,
			std::string_view culture_key, ovdl::v2script::ast::Node const* node
		);

	public:
		CultureManager();

		bool add_graphical_culture_type(std::string_view identifier);

		bool add_culture_group(
			std::string_view identifier, std::string_view leader, GraphicalCultureType const* graphical_culture_type,
			bool is_overseas, std::optional<country_index_t> union_country
		);

		bool add_culture(
			std::string_view identifier, colour_t colour, CultureGroup const& group, memory::vector<memory::string>&& first_names,
			memory::vector<memory::string>&& last_names, fixed_point_t radicalism, std::optional<country_index_t> primary_country
		);

		bool load_graphical_culture_type_file(ovdl::v2script::ast::Node const* root);
		bool load_culture_file(CountryDefinitionManager const& country_definition_manager, ovdl::v2script::ast::Node const* root);

		static memory::string make_leader_picture_name(
			std::string_view cultural_type, unit_branch_t branch, leader_count_t count
		);
		static memory::string make_leader_picture_path(std::string_view leader_picture_name);

		bool find_cultural_leader_pictures(Dataloader const& dataloader);

		memory::string get_leader_picture_name(std::string_view cultural_type, unit_branch_t branch) const;
	};
}
