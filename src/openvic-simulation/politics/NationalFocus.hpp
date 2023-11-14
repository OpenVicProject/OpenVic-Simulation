#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/Modifier.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/economy/Good.hpp"

#include <optional>

namespace OpenVic {
	struct NationalFocusManager;

	struct NationalFocusGroup : HasIdentifier {
		friend struct NationalFocusManager;
	
	private:
		NationalFocusGroup(std::string_view new_identifier);
	};

	struct NationalFocus : HasIdentifier {
		friend struct NationalFocusManager;

	public:
		using pop_promotion_map_t = std::map<PopType const*, fixed_point_t>;
		using party_loyalty_map_t = std::map<Ideology const*, fixed_point_t>;
		using production_map_t = std::map<Good const*, fixed_point_t>;

	private:
		uint8_t PROPERTY(icon);
		NationalFocusGroup const& PROPERTY(group);
		ModifierValue PROPERTY(modifiers);
		pop_promotion_map_t PROPERTY(encouraged_promotion);
		party_loyalty_map_t PROPERTY(encouraged_loyalty);
		production_map_t PROPERTY(encouraged_production);

		NationalFocus(
			std::string_view new_identifier,
			uint8_t new_icon,
			NationalFocusGroup const& new_group,
			ModifierValue&& new_modifiers, 
			pop_promotion_map_t&& new_encouraged_promotion, 
			party_loyalty_map_t&& new_encouraged_loyalty,
			production_map_t&& new_encouraged_production
		);

	public:
		NationalFocus(NationalFocus&&) = default;
	};

	struct NationalFocusManager {
	private:
		IdentifierRegistry<NationalFocusGroup> national_focus_groups;
		IdentifierRegistry<NationalFocus> national_foci;

	public:
		NationalFocusManager();

		inline bool add_national_focus_group(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(national_focus_group)

		inline bool add_national_focus(
			std::string_view identifier,
			uint8_t icon,
			NationalFocusGroup const& group,
			ModifierValue&& modifiers, 
			NationalFocus::pop_promotion_map_t&& encouraged_promotion, 
			NationalFocus::party_loyalty_map_t&& encouraged_loyalty,
			NationalFocus::production_map_t&& encouraged_production
		);
		IDENTIFIER_REGISTRY_ACCESSORS_CUSTOM_PLURAL(national_focus, national_foci);

		bool load_national_foci_file(PopManager const& pop_manager, IdeologyManager const& ideology_manager, GoodManager const& good_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
} // namespace OpenVic
