#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct IdeologyManager;

	struct IdeologyGroup : HasIdentifier {
		friend struct IdeologyManager;

	private:
		IdeologyGroup(std::string_view new_identifier);

	public:
		IdeologyGroup(IdeologyGroup&&) = default;
	};

	struct Ideology : HasIdentifierAndColour {
		friend struct IdeologyManager;

	private:
		IdeologyGroup const& group;
		const bool uncivilised, can_reduce_militancy;
		const Date spawn_date;

		//TODO - willingness to repeal/pass reforms (and its modifiers)

		Ideology(std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool new_uncivilised, bool new_can_reduce_militancy, Date new_spawn_date);

	public:
		Ideology(Ideology&&) = default;

		IdeologyGroup const& get_group() const;
		bool is_uncivilised() const;
		bool get_can_reduce_militancy() const;
		Date const& get_spawn_date() const;
	};

	struct IdeologyManager {
	private:
		IdentifierRegistry<IdeologyGroup> ideology_groups;
		IdentifierRegistry<Ideology> ideologies;

	public:
		IdeologyManager();

		bool add_ideology_group(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(ideology_group)

		bool add_ideology(std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised, bool can_reduce_militancy, Date spawn_date);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(ideology, ideologies)

		bool load_ideology_file(ast::NodeCPtr root);
	};
}