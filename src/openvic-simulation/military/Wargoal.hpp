#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct WargoalTypeManager;

	enum class peace_options_t : uint32_t {
		PO_ANNEX =				0b100000000000000000,
		PO_DEMAND_STATE =		0b010000000000000000,
		PO_COLONY =				0b001000000000000000,
		PO_ADD_TO_SPHERE =		0b000100000000000000,
		PO_DISARMAMENT =		0b000010000000000000,
		PO_REMOVE_FORTS =		0b000001000000000000,
		PO_REMOVE_NAVAL_BASES =	0b000000100000000000,
		PO_REPARATIONS =		0b000000010000000000,
		PO_REPAY_DEBT =			0b000000001000000000,
		PO_REMOVE_PRESTIGE =	0b000000000100000000,
		PO_MAKE_PUPPET =		0b000000000010000000,
		PO_RELEASE_PUPPET =		0b000000000001000000,
		PO_STATUS_QUO =			0b000000000000100000,
		PO_INSTALL_COMMUNISM =	0b000000000000010000,
		PO_REMOVE_COMMUNISM =	0b000000000000001000,
		PO_REMOVE_CORES =		0b000000000000000100, // only usable with ANNEX, DEMAND_STATE, or TRANSFER_PROVINCES
		PO_TRANSFER_PROVINCES =	0b000000000000000010,
		PO_CLEAR_UNION_SPHERE =	0b000000000000000001
	};
	template<> struct enable_bitfield<peace_options_t> : std::true_type{};

	struct WargoalType : HasIdentifier {
		friend struct WargoalTypeManager;

		enum class PEACE_MODIFIERS {
			BADBOY_FACTOR,
			PRESTIGE_FACTOR,
			PEACE_COST_FACTOR,
			PENALTY_FACTOR,
			BREAK_TRUCE_PRESTIGE_FACTOR,
			BREAK_TRUCE_INFAMY_FACTOR,
			BREAK_TRUCE_MILITANCY_FACTOR,
			GOOD_RELATION_PRESTIGE_FACTOR,
			GOOD_RELATION_INFAMY_FACTOR,
			GOOD_RELATION_MILITANCY_FACTOR,
			WAR_SCORE_BATTLE_FACTOR,
			CONSTRUCTION_SPEED
		};
		using peace_modifiers_t = fixed_point_map_t<PEACE_MODIFIERS>;

	private:
		const std::string PROPERTY(sprite);
		const std::string PROPERTY(war_name);
		const Timespan PROPERTY(available_length);
		const Timespan PROPERTY(truce_length);
		const bool PROPERTY(triggered_only); // only able to be added via effects (or within the code)
		const bool PROPERTY(civil_war);
		const bool PROPERTY(constructing); // can be added to existing wars or justified
		const bool PROPERTY(crisis); // able to be added to crises
		const bool PROPERTY(great_war); // automatically add to great wars
		const bool PROPERTY(mutual); // attacked and defender share wargoal
		const peace_modifiers_t PROPERTY(modifiers);
		const peace_options_t PROPERTY(peace_options);

		// TODO: can_use, prerequisites, on_add, on_po_accepted

		WargoalType(
			std::string_view new_identifier,
			std::string_view new_sprite,
			std::string_view new_war_name,
			Timespan new_available_length,
			Timespan new_truce_length,
			bool new_triggered_only,
			bool new_civil_war,
			bool new_constructing,
			bool new_crisis,
			bool new_great_war,
			bool new_mutual,
			const peace_modifiers_t&& new_modifiers,
			peace_options_t new_peace_options
		);

	public:
		WargoalType(WargoalType&&) = default;
	};

	struct WargoalTypeManager {
	private:
		IdentifierRegistry<WargoalType> wargoal_types;
		std::vector<WargoalType const*> PROPERTY(peace_priorities);

	public:
		WargoalTypeManager();

		const std::vector<WargoalType const*>& get_peace_priority_list() const;

		bool add_wargoal_type(
			std::string_view identifier,
			std::string_view sprite,
			std::string_view war_name,
			Timespan available_length,
			Timespan truce_length,
			bool triggered_only,
			bool civil_war,
			bool constructing,
			bool crisis,
			bool great_war,
			bool mutual,
			WargoalType::peace_modifiers_t&& modifiers,
			peace_options_t peace_options
		);
		IDENTIFIER_REGISTRY_ACCESSORS(wargoal_type)

		bool load_wargoal_file(ast::NodeCPtr root);
	};
} // namespace OpenVic