#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

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
		using pop_promotion_map_t = fixed_point_map_t<PopType const*>;
		using production_map_t = fixed_point_map_t<Good const*>;

	private:
		uint8_t PROPERTY(icon);
		NationalFocusGroup const& PROPERTY(group);
		ModifierValue PROPERTY(modifiers);
		pop_promotion_map_t PROPERTY(encouraged_promotion);
		production_map_t PROPERTY(encouraged_production);
		Ideology const* PROPERTY(loyalty_ideology);
		fixed_point_t PROPERTY(loyalty_value);
		ConditionScript PROPERTY(limit);

		NationalFocus(
			std::string_view new_identifier,
			uint8_t new_icon,
			NationalFocusGroup const& new_group,
			ModifierValue&& new_modifiers,
			pop_promotion_map_t&& new_encouraged_promotion,
			production_map_t&& new_encouraged_production,
			Ideology const* new_loyalty_ideology,
			fixed_point_t new_loyalty_value,
			ConditionScript&& new_limit
		);

		bool parse_scripts(GameManager const& game_manager);

	public:
		NationalFocus(NationalFocus&&) = default;
	};

	struct NationalFocusManager {
	private:
		IdentifierRegistry<NationalFocusGroup> IDENTIFIER_REGISTRY(national_focus_group);
		IdentifierRegistry<NationalFocus> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(national_focus, national_foci);

	public:
		inline bool add_national_focus_group(std::string_view identifier);

		inline bool add_national_focus(
			std::string_view identifier,
			uint8_t icon,
			NationalFocusGroup const& group,
			ModifierValue&& modifiers,
			NationalFocus::pop_promotion_map_t&& encouraged_promotion,
			NationalFocus::production_map_t&& encouraged_production,
			Ideology const* loyalty_ideology,
			fixed_point_t loyalty_value,
			ConditionScript&& limit
		);

		bool load_national_foci_file(
			PopManager const& pop_manager, IdeologyManager const& ideology_manager, GoodManager const& good_manager,
			ModifierManager const& modifier_manager, ast::NodeCPtr root
		);

		bool parse_scripts(GameManager const& game_manager);
	};
} // namespace OpenVic
