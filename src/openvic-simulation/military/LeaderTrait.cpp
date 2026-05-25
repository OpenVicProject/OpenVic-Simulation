#include "LeaderTrait.hpp"

using namespace OpenVic;

LeaderTrait::LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers)
	: Modifier { new_identifier, std::move(new_modifiers), modifier_type_t::LEADER }, trait_type { new_type } {}

bool LeaderTrait::is_personality_trait() const {
	return trait_type == trait_type_t::PERSONALITY;
}

bool LeaderTrait::is_background_trait() const {
	return trait_type == trait_type_t::BACKGROUND;
}
