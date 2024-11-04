#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

	struct CultureManager;
	struct CountryDefinition;
	struct CountryDefinitionManager;

	struct GraphicalCultureType : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType(std::string_view new_identifier);

	public:
		GraphicalCultureType(GraphicalCultureType&&) = default;
	};

	struct CultureGroup : HasIdentifier {
		friend struct CultureManager;

	private:
		std::string PROPERTY(leader);
		GraphicalCultureType const& PROPERTY(unit_graphical_culture_type);
		bool PROPERTY(is_overseas);
		CountryDefinition const* PROPERTY(union_country);

		CultureGroup(
			std::string_view new_identifier, std::string_view new_leader,
			GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas,
			CountryDefinition const* new_union_country
		);

	public:
		CultureGroup(CultureGroup&&) = default;

		constexpr bool has_union_country() const {
			return union_country != nullptr;
		}
	};

	struct Culture : HasIdentifierAndColour {
		friend struct CultureManager;

	private:
		CultureGroup const& PROPERTY(group);
		name_list_t PROPERTY(first_names);
		name_list_t PROPERTY(last_names);
		fixed_point_t PROPERTY(radicalism);
		CountryDefinition const* PROPERTY(primary_country);

		Culture(
			std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, name_list_t&& new_first_names,
			name_list_t&& new_last_names, fixed_point_t new_radicalism, CountryDefinition const* new_primary_country
		);

	public:
		Culture(Culture&&) = default;

		constexpr bool has_union_country() const {
			return group.has_union_country();
		}
	};

	struct CultureManager {
	private:
		IdentifierRegistry<GraphicalCultureType> IDENTIFIER_REGISTRY(graphical_culture_type);
		IdentifierRegistry<CultureGroup> IDENTIFIER_REGISTRY(culture_group);
		IdentifierRegistry<Culture> IDENTIFIER_REGISTRY(culture);

		GraphicalCultureType const* PROPERTY(default_graphical_culture_type);

		bool _load_culture_group(
			CountryDefinitionManager const& country_definition_manager, size_t& total_expected_cultures,
			std::string_view culture_group_key, ast::NodeCPtr culture_group_node
		);
		bool _load_culture(
			CountryDefinitionManager const& country_definition_manager, CultureGroup const& culture_group,
			std::string_view culture_key, ast::NodeCPtr node
		);

	public:
		CultureManager();

		bool add_graphical_culture_type(std::string_view identifier);

		bool add_culture_group(
			std::string_view identifier, std::string_view leader, GraphicalCultureType const* graphical_culture_type,
			bool is_overseas, CountryDefinition const* union_country
		);

		bool add_culture(
			std::string_view identifier, colour_t colour, CultureGroup const& group, name_list_t&& first_names,
			name_list_t&& last_names, fixed_point_t radicalism, CountryDefinition const* primary_country
		);

		bool load_graphical_culture_type_file(ast::NodeCPtr root);
		bool load_culture_file(CountryDefinitionManager const& country_definition_manager, ast::NodeCPtr root);
	};
}
