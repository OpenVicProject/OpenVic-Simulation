#pragma once

#include "openvic/types/IdentifierRegistry.hpp"
#include "openvic/dataloader/NodeTools.hpp"

namespace OpenVic {

	struct ReligionManager;

	struct ReligionGroup : HasIdentifier {
		friend struct ReligionManager;

	private:
		ReligionGroup(const std::string_view new_identifier);

	public:
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifierAndColour {
		friend struct ReligionManager;

		using icon_t = uint8_t;

	private:
		ReligionGroup const& group;
		const icon_t icon;
		const bool pagan;

		Religion(const std::string_view new_identifier, colour_t new_colour, ReligionGroup const& new_group, icon_t new_icon, bool new_pagan);

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

		return_t add_religion_group(const std::string_view identifier);
		void lock_religion_groups();
		ReligionGroup const* get_religion_group_by_identifier(const std::string_view identifier) const;
		return_t add_religion(const std::string_view identifier, colour_t colour, ReligionGroup const* group, Religion::icon_t icon, bool pagan);
		void lock_religions();
		Religion const* get_religion_by_identifier(const std::string_view identifier) const;

		return_t load_religion_file(ast::NodeCPtr root);
	};
}
