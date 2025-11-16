#pragma once

#include <string_view>

#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct UnitType;
	struct LeaderTraitManager;

	struct Modifier : HasIdentifier, ModifierValue {
		friend struct ModifierManager;
		friend struct StaticModifierCache;
		friend struct UnitType;
		friend struct LeaderTraitManager;

		enum struct modifier_type_t : uint8_t {
			EVENT, STATIC, TRIGGERED, CRIME, TERRAIN, CLIMATE, CONTINENT, BUILDING, LEADER, UNIT_TERRAIN,
			NATIONAL_VALUE, NATIONAL_FOCUS, PARTY_POLICY, REFORM, TECHNOLOGY, INVENTION, TECH_SCHOOL
		};

		static std::string_view modifier_type_to_string(modifier_type_t type);

	protected:
		Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type);

	public:
		const modifier_type_t type;

		Modifier(Modifier&&) = default;
	};

	struct IconModifier : Modifier {
		using icon_t = uint8_t;

	public:
		/* A modifier can have no icon (zero). */
		const icon_t icon;

		IconModifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon);
		IconModifier(IconModifier&&) = default;
	};

	struct TriggeredModifier : IconModifier {
		friend struct ModifierManager;

	private:
		ConditionScript trigger;

	protected:
		bool parse_scripts(DefinitionManager const& definition_manager, bool can_be_null = false);

	public:
		TriggeredModifier(
			std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
			ConditionScript&& new_trigger
		);
		TriggeredModifier(TriggeredModifier&&) = default;
	};

	struct ModifierInstance {

	private:
		Modifier const* PROPERTY(modifier); // We can assume this is never null
		Date PROPERTY(expiry_date);

	public:
		ModifierInstance(Modifier const& new_modifier, Date new_expiry_date);
	};
}
