#pragma once

#include "../Types.hpp"

namespace OpenVic {

	struct CultureManager;

	struct GraphicalCultureType : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType(std::string const& new_identifier);

	public:
		GraphicalCultureType(GraphicalCultureType&&) = default;
	};

	struct CultureGroup : HasIdentifier {
		friend struct CultureManager;

	private:
		GraphicalCultureType const& unit_graphical_culture_type;

		// TODO - leader type, union tag

		CultureGroup(std::string const& new_identifier, GraphicalCultureType const& new_unit_graphical_culture_type);

	public:
		CultureGroup(CultureGroup&&) = default;

		GraphicalCultureType const& get_unit_graphical_culture_type() const;
	};

	struct Culture : HasIdentifier, HasColour {
		friend struct CultureManager;

		using name_list_t = std::vector<std::string>;

	private:
		CultureGroup const& group;
		const name_list_t first_names, last_names;

		// TODO - radicalism, primary tag

		Culture(CultureGroup const& new_group, std::string const& new_identifier, colour_t new_colour, name_list_t const& new_first_names, name_list_t const& new_last_names);

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

		return_t add_graphical_culture_type(std::string const& identifier);
		void lock_graphical_culture_types();
		GraphicalCultureType const* get_graphical_culture_type_by_identifier(std::string const& identifier) const;
		return_t add_culture_group(std::string const& identifier, GraphicalCultureType const* new_graphical_culture_type);
		void lock_culture_groups();
		CultureGroup const* get_culture_group_by_identifier(std::string const& identifier) const;
		return_t add_culture(std::string const& identifier, colour_t colour, CultureGroup const* group, Culture::name_list_t const& first_names, Culture::name_list_t const& last_names);
		void lock_cultures();
		Culture const* get_culture_by_identifier(std::string const& identifier) const;
	};
}
