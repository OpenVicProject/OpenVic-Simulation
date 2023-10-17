#include "LeaderTrait.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

LeaderTrait::LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers)
	: HasIdentifier { new_identifier }, type { new_type }, modifiers { std::move(new_modifiers) } {}

LeaderTrait::trait_type_t LeaderTrait::get_trait_type() const {
	return type;
}

bool LeaderTrait::is_personality_trait() const {
	return type == trait_type_t::PERSONALITY;
}

bool LeaderTrait::is_background_trait() const {
	return type == trait_type_t::BACKGROUND;
}

ModifierValue const& LeaderTrait::get_modifiers() const {
	return modifiers;
}

LeaderTraitManager::LeaderTraitManager() : leader_traits { "leader trait" } {}

bool LeaderTraitManager::add_leader_trait(std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers) {
	if (identifier.empty()) {
		Logger::error("Invalid leader trait identifier - empty!");
		return false;
	}

	return leader_traits.add_item({ identifier, type, std::move(modifiers) });
}

bool LeaderTraitManager::load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	using enum LeaderTrait::trait_type_t;
	const auto trait_callback = [this, &modifier_manager](LeaderTrait::trait_type_t type) -> key_value_callback_t {
		return [this, &modifier_manager, type](std::string_view trait_identifier, ast::NodeCPtr value) -> bool {
			ModifierValue modifiers;
			bool ret = modifier_manager.expect_whitelisted_modifier_value(move_variable_callback(modifiers), allowed_modifiers)(value);
			ret &= add_leader_trait(trait_identifier, type, std::move(modifiers));
			return ret;
		};
	};
	const bool ret = expect_dictionary_keys(
		"personality", ONE_EXACTLY, expect_dictionary(trait_callback(PERSONALITY)),
		"background", ONE_EXACTLY, expect_dictionary(trait_callback(BACKGROUND))
	)(root);
	lock_leader_traits();

	return ret;
}
