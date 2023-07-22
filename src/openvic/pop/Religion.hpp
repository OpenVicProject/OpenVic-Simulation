#pragma once

#include "../Types.hpp"

namespace OpenVic {

	struct ReligionManager;

	struct ReligionGroup : HasIdentifier {
		friend struct ReligionManager;

	private:
		ReligionGroup(std::string const& new_identifier);

	public:
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifier, HasColour {
		friend struct ReligionManager;

		using icon_t = uint8_t;

	private:
		ReligionGroup const& group;
		const icon_t icon;
		const bool pagan;

		Religion(ReligionGroup const& new_group, std::string const& new_identifier, colour_t new_colour, icon_t new_icon, bool new_pagan);

	public:
		Religion(Religion&&) = default;

		ReligionGroup const& get_group() const;
		icon_t get_icon() const;
		bool get_pagan() const;
	};

	struct ReligionManager {
	private:
		IdentifierRegistry<ReligionGroup> religion_groups;
		IdentifierRegistry<Religion> religions;

	public:
		ReligionManager();

		return_t add_religion_group(std::string const& identifier);
		void lock_religion_groups();
		ReligionGroup const* get_religion_group_by_identifier(std::string const& identifier) const;
		return_t add_religion(ReligionGroup const* group, std::string const& identifier, colour_t colour, Religion::icon_t icon, bool pagan);
		void lock_religions();
		Religion const* get_religion_by_identifier(std::string const& identifier) const;
	};
}
