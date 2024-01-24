#include "LeaderTrait.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

LeaderTrait::LeaderTrait(std::string_view new_identifier, trait_type_t new_type, ModifierValue&& new_modifiers)
	: HasIdentifier { new_identifier }, trait_type { new_type }, modifiers { std::move(new_modifiers) } {}

bool LeaderTrait::is_personality_trait() const {
	return trait_type == trait_type_t::PERSONALITY;
}

bool LeaderTrait::is_background_trait() const {
	return trait_type == trait_type_t::BACKGROUND;
}

bool LeaderTraitManager::add_leader_trait(
	std::string_view identifier, LeaderTrait::trait_type_t type, ModifierValue&& modifiers
) {
	if (identifier.empty()) {
		Logger::error("Invalid leader trait identifier - empty!");
		return false;
	}

	return leader_traits.add_item({ identifier, type, std::move(modifiers) });
}

bool LeaderTraitManager::load_leader_traits_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	const auto trait_callback = [this, &modifier_manager](LeaderTrait::trait_type_t type) -> NodeCallback auto {
		return expect_dictionary_reserve_length(
			leader_traits,
			[this, &modifier_manager, type](std::string_view trait_identifier, ast::NodeCPtr value) -> bool {
				static const string_set_t allowed_modifiers = {
					"attack", "defence", "morale", "organisation", "reconnaissance",
					"speed", "attrition", "experience", "reliability"
				};

				ModifierValue modifiers;
				bool ret = modifier_manager.expect_whitelisted_modifier_value(
					move_variable_callback(modifiers), allowed_modifiers
				)(value);

				ret &= add_leader_trait(trait_identifier, type, std::move(modifiers));
				return ret;
			}
		);
	};

	using enum LeaderTrait::trait_type_t;

	const bool ret = expect_dictionary_keys(
		"personality", ONE_EXACTLY, trait_callback(PERSONALITY),
		"background", ONE_EXACTLY, trait_callback(BACKGROUND)
	)(root);

	lock_leader_traits();

	return ret;
}
