#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

	struct CultureManager;

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
		const std::string PROPERTY(leader);
		GraphicalCultureType const& PROPERTY(unit_graphical_culture_type);
		const bool PROPERTY(is_overseas);

		// TODO - union tag

		CultureGroup(
			std::string_view new_identifier, std::string_view new_leader,
			GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas
		);

	public:
		CultureGroup(CultureGroup&&) = default;
	};

	struct Culture : HasIdentifierAndColour {
		friend struct CultureManager;

	private:
		CultureGroup const& PROPERTY(group);
		const name_list_t PROPERTY(first_names);
		const name_list_t PROPERTY(last_names);

		// TODO - radicalism, primary tag

		Culture(
			std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group,
			name_list_t&& new_first_names, name_list_t&& new_last_names
		);

	public:
		Culture(Culture&&) = default;
	};

	struct CultureManager {
	private:
		IdentifierRegistry<GraphicalCultureType> IDENTIFIER_REGISTRY(graphical_culture_type);
		IdentifierRegistry<CultureGroup> IDENTIFIER_REGISTRY(culture_group);
		IdentifierRegistry<Culture> IDENTIFIER_REGISTRY(culture);

		bool _load_culture_group(
			size_t& total_expected_cultures, GraphicalCultureType const* default_unit_graphical_culture_type,
			std::string_view culture_group_key, ast::NodeCPtr culture_group_node
		);
		bool _load_culture(CultureGroup const& culture_group, std::string_view culture_key, ast::NodeCPtr node);

	public:
		bool add_graphical_culture_type(std::string_view identifier);

		bool add_culture_group(
			std::string_view identifier, std::string_view leader, GraphicalCultureType const* graphical_culture_type,
			bool is_overseas
		);

		bool add_culture(
			std::string_view identifier, colour_t colour, CultureGroup const& group, name_list_t&& first_names,
			name_list_t&& last_names
		);

		bool load_graphical_culture_type_file(ast::NodeCPtr root);
		bool load_culture_file(ast::NodeCPtr root);
	};
}
