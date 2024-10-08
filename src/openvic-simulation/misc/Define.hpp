#pragma once

#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct DefineManager;

	struct Define : HasIdentifier {
		friend struct DefineManager;

		enum class Type : unsigned char { Unknown, Date, Country, Economy, Military, Diplomacy, Pops, Ai, Graphics };

		static std::string_view type_to_string(Type type);
		// This only accepts type names found in defines.lua, so it will never return Type::Date
		static Type string_to_type(std::string_view str);

	private:
		std::string PROPERTY(value);
		const Type PROPERTY(type);

		Define(std::string_view new_identifier, std::string_view new_value, Type new_type);

	public:
		Define(Define&&) = default;

		Date get_value_as_date(bool* successful = nullptr) const;
		fixed_point_t get_value_as_fp(bool* successful = nullptr) const;
		int64_t get_value_as_int(bool* successful = nullptr) const;
		uint64_t get_value_as_uint(bool* successful = nullptr) const;
	};

	std::ostream& operator<<(std::ostream& os, Define::Type type);

	struct DefineManager {
	private:
		IdentifierRegistry<Define> IDENTIFIER_REGISTRY(define);

		// Date
		Date PROPERTY(start_date); // start_date
		Date PROPERTY(end_date); // end_date

		// Country
		size_t PROPERTY(great_power_rank); // GREAT_NATIONS_COUNT
		Timespan PROPERTY(lose_great_power_grace_days); // GREATNESS_DAYS
		size_t PROPERTY(secondary_power_rank); // COLONIAL_RANK
		fixed_point_t PROPERTY(country_investment_industrial_score_factor); // INVESTMENT_SCORE_FACTOR

		// Economy

		// Military
		Pop::pop_size_t PROPERTY(pop_size_per_regiment); // POP_SIZE_PER_REGIMENT
		Pop::pop_size_t PROPERTY(min_pop_size_for_regiment); // POP_MIN_SIZE_FOR_REGIMENT
		// POP_MIN_SIZE_FOR_REGIMENT_PROTECTORATE_MULTIPLIER
		fixed_point_t PROPERTY(pop_size_per_regiment_protectorate_multiplier);
		fixed_point_t PROPERTY(pop_size_per_regiment_colony_multiplier); // POP_MIN_SIZE_FOR_REGIMENT_COLONY_MULTIPLIER
		fixed_point_t PROPERTY(pop_size_per_regiment_non_core_multiplier); // POP_MIN_SIZE_FOR_REGIMENT_NONCORE_MULTIPLIER

		// Diplomacy
		fixed_point_t PROPERTY(disarmed_penalty); // DISARMAMENT_ARMY_HIT

		// Pops

		// Ai

		// Graphics

		template<typename T>
		bool load_define(T& value, Define::Type type, std::string_view name) const;

		template<Timespan (*Func)(Timespan::day_t)>
		bool _load_define_timespan(Timespan& value, Define::Type type, std::string_view name) const;

		bool load_define_days(Timespan& value, Define::Type type, std::string_view name) const;
		bool load_define_months(Timespan& value, Define::Type type, std::string_view name) const;
		bool load_define_years(Timespan& value, Define::Type type, std::string_view name) const;

	public:
		DefineManager();

		bool add_define(std::string_view name, std::string_view value, Define::Type type);

		bool in_game_period(Date date) const;

		bool load_defines_file(ast::NodeCPtr root);
	};
}
