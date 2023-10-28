#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"

namespace OpenVic {

	struct PopManager;
	struct PopType;

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop {
		friend struct PopManager;

		using pop_size_t = int64_t;

	private:
		PopType const& type;
		Culture const& culture;
		Religion const& religion;
		pop_size_t size, num_promoted, num_demoted, num_migrated;

		Pop(PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size);

	public:
		Pop(Pop const&) = delete;
		Pop(Pop&&) = default;
		Pop& operator=(Pop const&) = delete;
		Pop& operator=(Pop&&) = delete;

		PopType const& get_type() const;
		Culture const& get_culture() const;
		Religion const& get_religion() const;
		pop_size_t get_size() const;
		pop_size_t get_num_promoted() const;
		pop_size_t get_num_demoted() const;
		pop_size_t get_num_migrated() const;
		pop_size_t get_pop_daily_change() const;
	};

	/* REQUIREMENTS:
	 * POP-15, POP-16, POP-17, POP-26
	 */
	struct PopType : HasIdentifierAndColour {
		friend struct PopManager;

		using sprite_t = uint8_t;
		using rebel_units_t = decimal_map_t<Unit const*>;

	private:
		const enum class strata_t {
			POOR,
			MIDDLE,
			RICH
		} strata;
		const sprite_t sprite;
		const Good::good_map_t life_needs, everyday_needs, luxury_needs;
		const rebel_units_t rebel_units;
		const Pop::pop_size_t max_size, merge_max_size;
		const bool state_capital_only, demote_migrant, is_artisan, is_slave;

		// TODO - country and province migration targets, promote_to targets, ideologies and issues

		PopType(std::string_view new_identifier, colour_t new_colour, strata_t new_strata, sprite_t new_sprite,
			Good::good_map_t&& new_life_needs, Good::good_map_t&& new_everyday_needs, Good::good_map_t&& new_luxury_needs,
			rebel_units_t&& new_rebel_units, Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
			bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_is_slave);

	public:
		PopType(PopType&&) = default;

		strata_t get_strata() const;
		sprite_t get_sprite() const;
		Good::good_map_t const& get_life_needs() const;
		Good::good_map_t const& get_everyday_needs() const;
		Good::good_map_t const& get_luxury_needs() const;
		rebel_units_t const& get_rebel_units() const;
		Pop::pop_size_t get_max_size() const;
		Pop::pop_size_t get_merge_max_size() const;
		bool get_state_capital_only() const;
		bool get_demote_migrant() const;
		bool get_is_artisan() const;
		bool get_is_slave() const;
	};

	struct Province;

	struct PopManager {
	private:
		IdentifierRegistry<PopType> pop_types;

		CultureManager culture_manager;
		ReligionManager religion_manager;

	public:
		PopManager();

		REF_GETTERS(culture_manager)
		REF_GETTERS(religion_manager)

		bool add_pop_type(std::string_view identifier, colour_t new_colour, PopType::strata_t strata, PopType::sprite_t sprite,
			Good::good_map_t&& life_needs, Good::good_map_t&& everyday_needs, Good::good_map_t&& luxury_needs,
			PopType::rebel_units_t&& rebel_units, Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size,
			bool state_capital_only, bool demote_migrant, bool is_artisan, bool is_slave);
		IDENTIFIER_REGISTRY_ACCESSORS(pop_type)

		bool load_pop_type_file(std::string_view filestem, UnitManager const& unit_manager,
			GoodManager const& good_manager, ast::NodeCPtr root);
		bool load_pop_into_province(Province& province, std::string_view pop_type_identifier, ast::NodeCPtr pop_node) const;
	};
}
