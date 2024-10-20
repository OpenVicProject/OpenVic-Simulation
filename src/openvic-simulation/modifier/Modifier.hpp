#pragma once

#include <string_view>

#include "openvic-simulation/modifier/ModifierValue.hpp"
#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct UnitType;

	struct Modifier : HasIdentifier, ModifierValue {
		friend struct ModifierManager;
		friend struct UnitType;

		enum struct modifier_type_t : uint8_t {
			EVENT, STATIC, TRIGGERED, CRIME, TERRAIN, CLIMATE, CONTINENT, BUILDING, LEADER, UNIT_TERRAIN,
			NATIONAL_VALUE, NATIONAL_FOCUS, ISSUE, REFORM, TECHNOLOGY, INVENTION, TECH_SCHOOL
		};

		static std::string_view modifier_type_to_string(modifier_type_t type);

	private:
		const modifier_type_t PROPERTY(type);

	protected:
		Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type);

	public:
		Modifier(Modifier&&) = default;
	};

	struct IconModifier : Modifier {
		friend struct ModifierManager;

		using icon_t = uint8_t;

	private:
		/* A modifier can have no icon (zero). */
		const icon_t PROPERTY(icon);

	protected:
		IconModifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon);

	public:
		IconModifier(IconModifier&&) = default;
	};

	struct TriggeredModifier : IconModifier {
		friend struct ModifierManager;

	private:
		ConditionScript trigger;

	protected:
		TriggeredModifier(
			std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
			ConditionScript&& new_trigger
		);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
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
