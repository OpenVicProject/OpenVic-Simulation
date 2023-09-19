#include "Ideology.hpp"
#include "types/IdentifierRegistry.hpp"

using namespace OpenVic;

IdeologyGroup::IdeologyGroup(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Ideology::Ideology(const std::string_view new_identifier, colour_t new_colour, IdeologyGroup const& new_group, bool uncivilised, Date spawn_date)
	: HasIdentifierAndColour { new_identifier, new_colour, true, false }, group { new_group }, uncivilised { uncivilised },
	spawn_date { spawn_date } {}

bool Ideology::is_uncivilised() const {
	return uncivilised;
}

Date const& Ideology::get_spawn_date() const {
	return spawn_date;
}

IdeologyManager::IdeologyManager() : ideology_groups { "ideology groups" }, ideologies { "ideologies" } {}

bool IdeologyManager::add_ideology_group(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology group identifier - empty!");
		return false;
	}

	return ideology_groups.add_item({ identifier });
}

bool IdeologyManager::add_ideology(const std::string_view identifier, colour_t colour, IdeologyGroup const* group, bool uncivilised, Date spawn_date) {
	if (identifier.empty()) {
		Logger::error("Invalid ideology identifier - empty!");
		return false;
	}

	if (colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid ideology colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}

	if (group == nullptr) {
		Logger::error("Null ideology group for ", identifier);
		return false;
	}

	return ideologies.add_item({ identifier, colour, *group, uncivilised, spawn_date });
}