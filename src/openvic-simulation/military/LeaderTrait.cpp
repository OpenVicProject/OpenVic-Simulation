#include "LeaderTrait.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

LeaderTrait::LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue new_modifiers)
    : HasIdentifier { new_identifier }, type { new_type }, modifiers { new_modifiers } {}

trait_type_t LeaderTrait::get_trait_type() const {
    return type;
}

bool LeaderTrait::is_personality_trait() const {
    return type == trait_type_t::PERSONALITY;
}

bool LeaderTrait::is_background_trait() const {
    return type == trait_type_t::BACKGROUND;
}

ModifierValue LeaderTrait::get_modifiers() const {
    return modifiers;
}

LeaderTraitManager::LeaderTraitManager() : leader_traits { "leader_traits" } {}

bool LeaderTraitManager::add_leader_trait(std::string_view identifier, trait_type_t type, ModifierValue modifiers) {
    if (identifier.empty()) {
        Logger::error("Invalid leader trait identifier - empty!");
        return false;
    }

    return leader_traits.add_item({ identifier, type, modifiers });
}

bool LeaderTraitManager::load_leader_traits_file(ModifierManager& modifier_manager, ast::NodeCPtr root) {
    bool ret = expect_dictionary_keys(
        "personality", ONE_EXACTLY, expect_dictionary(
            [this, &modifier_manager](std::string_view trait_identifier, ast::NodeCPtr value) -> bool {
                ModifierValue modifiers;

                bool ret = modifier_manager.expect_whitelisted_modifier_value(move_variable_callback(modifiers), allowed_modifiers)(value);
                ret &= add_leader_trait(trait_identifier, trait_type_t::PERSONALITY, modifiers);
                return ret;
            }
        ),
        "background", ONE_EXACTLY, expect_dictionary(
            [this, &modifier_manager](std::string_view trait_identifier, ast::NodeCPtr value) -> bool {
                ModifierValue modifiers;

                bool ret = modifier_manager.expect_whitelisted_modifier_value(move_variable_callback(modifiers), allowed_modifiers)(value);
                ret &= add_leader_trait(trait_identifier, trait_type_t::BACKGROUND, modifiers);
                return ret;
            }
        )
    )(root);
    lock_leader_traits();

    return ret;
}