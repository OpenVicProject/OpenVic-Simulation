#pragma once

#include "openvic/types/IdentifierRegistry.hpp"
#include "openvic/dataloader/NodeTools.hpp"

namespace OpenVic {

	struct CultureManager;

	struct GraphicalCultureType : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType(const std::string_view new_identifier);

	public:
		GraphicalCultureType(GraphicalCultureType&&) = default;
	};

	struct CultureGroup : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType const& unit_graphical_culture_type;
		bool is_overseas;

		// TODO - leader type, union tag

		CultureGroup(const std::string_view new_identifier, GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas);

	public:
		CultureGroup(CultureGroup&&) = default;

		GraphicalCultureType const& get_unit_graphical_culture_type() const;
	};

	struct Culture : HasIdentifierAndColour {
		friend struct CultureManager;

		using name_list_t = std::vector<std::string>;

	private:
		CultureGroup const& group;
		const name_list_t first_names, last_names;

		// TODO - radicalism, primary tag

		Culture(const std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, name_list_t const& new_first_names, name_list_t const& new_last_names);

	public:
		Culture(Culture&&) = default;

		CultureGroup const& get_group() const;
	};

	struct CultureManager {
	private:
		IdentifierRegistry<GraphicalCultureType> graphical_culture_types;
		IdentifierRegistry<CultureGroup> culture_groups;
		IdentifierRegistry<Culture> cultures;

	public:
		CultureManager();

		return_t add_graphical_culture_type(const std::string_view identifier);
		void lock_graphical_culture_types();
		GraphicalCultureType const* get_graphical_culture_type_by_identifier(const std::string_view identifier) const;
		return_t add_culture_group(const std::string_view identifier, GraphicalCultureType const* new_graphical_culture_type, bool is_overseas);
		void lock_culture_groups();
		CultureGroup const* get_culture_group_by_identifier(const std::string_view identifier) const;
		return_t add_culture(const std::string_view identifier, colour_t colour, CultureGroup const* group, Culture::name_list_t const& first_names, Culture::name_list_t const& last_names);
		void lock_cultures();
		Culture const* get_culture_by_identifier(const std::string_view identifier) const;

		return_t load_graphical_culture_type_file(ast::NodeCPtr node);
		return_t load_culture_file(ast::NodeCPtr node);
	};
}
