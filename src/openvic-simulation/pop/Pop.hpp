#pragma once

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/pop/Religion.hpp"

namespace OpenVic {

	struct PopManager;
	struct PopType;
	struct RebelType;
	struct RebelManager;

	/* REQUIREMENTS:
	 * POP-18, POP-19, POP-20, POP-21, POP-34, POP-35, POP-36, POP-37
	 */
	struct Pop {
		friend struct PopManager;

		using pop_size_t = int64_t;

	private:
		PopType const& PROPERTY(type);
		Culture const& PROPERTY(culture);
		Religion const& PROPERTY(religion);
		pop_size_t PROPERTY(size);
		pop_size_t PROPERTY(num_promoted);
		pop_size_t PROPERTY(num_demoted);
		pop_size_t PROPERTY(num_migrated);

		fixed_point_t PROPERTY(militancy);
		fixed_point_t PROPERTY(consciousness);
		RebelType const* PROPERTY(rebel_type);

		Pop(
			PopType const& new_type, Culture const& new_culture, Religion const& new_religion, pop_size_t new_size,
			fixed_point_t new_militancy, fixed_point_t new_consciousness, RebelType const* new_rebel_type
		);

	public:
		Pop(Pop const&) = default;
		Pop(Pop&&) = default;
		Pop& operator=(Pop const&) = delete;
		Pop& operator=(Pop&&) = delete;

		pop_size_t get_pop_daily_change() const;
	};

	/* REQUIREMENTS:
	 * POP-15, POP-16, POP-17, POP-26
	 */
	struct PopType : HasIdentifierAndColour {
		friend struct PopManager;

		using sprite_t = uint8_t;
		using rebel_units_t = fixed_point_map_t<Unit const*>;

	private:
		const enum class strata_t { POOR, MIDDLE, RICH } PROPERTY(strata);
		const sprite_t PROPERTY(sprite);
		const Good::good_map_t PROPERTY(life_needs);
		const Good::good_map_t PROPERTY(everyday_needs);
		const Good::good_map_t PROPERTY(luxury_needs);
		const rebel_units_t PROPERTY(rebel_units);
		const Pop::pop_size_t PROPERTY(max_size);
		const Pop::pop_size_t PROPERTY(merge_max_size);
		const bool PROPERTY(state_capital_only);
		const bool PROPERTY(demote_migrant);
		const bool PROPERTY(is_artisan);
		const bool PROPERTY(allowed_to_vote);
		const bool PROPERTY(is_slave);
		const bool PROPERTY(can_be_recruited);
		const bool PROPERTY(can_reduce_consciousness);
		const bool PROPERTY(administrative_efficiency);
		const bool PROPERTY(can_build);
		const bool PROPERTY(factory);
		const bool PROPERTY(can_work_factory);
		const bool PROPERTY(unemployment);

		// TODO - country and province migration targets, promote_to targets, ideologies and issues

		PopType(
			std::string_view new_identifier, colour_t new_colour, strata_t new_strata, sprite_t new_sprite,
			Good::good_map_t&& new_life_needs, Good::good_map_t&& new_everyday_needs, Good::good_map_t&& new_luxury_needs,
			rebel_units_t&& new_rebel_units, Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
			bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_allowed_to_vote,
			bool new_is_slave, bool new_can_be_recruited, bool new_can_reduce_consciousness,
			bool new_administrative_efficiency, bool new_can_build, bool new_factory, bool new_can_work_factory,
			bool new_unemployment
		);

	public:
		PopType(PopType&&) = default;
	};

	struct Province;

	struct PopManager {
	private:
		IdentifierRegistry<PopType> IDENTIFIER_REGISTRY(pop_type);
		PopType::sprite_t PROPERTY(slave_sprite);
		PopType::sprite_t PROPERTY(administrative_sprite);

		CultureManager PROPERTY_REF(culture_manager);
		ReligionManager PROPERTY_REF(religion_manager);

	public:
		PopManager();

		bool add_pop_type(
			std::string_view identifier, colour_t new_colour, PopType::strata_t strata, PopType::sprite_t sprite,
			Good::good_map_t&& life_needs, Good::good_map_t&& everyday_needs, Good::good_map_t&& luxury_needs,
			PopType::rebel_units_t&& rebel_units, Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size,
			bool state_capital_only, bool demote_migrant, bool is_artisan, bool allowed_to_vote, bool is_slave,
			bool can_be_recruited, bool can_reduce_consciousness, bool administrative_efficiency, bool can_build, bool factory,
			bool can_work_factory, bool unemployment
		);

		bool load_pop_type_file(
			std::string_view filestem, UnitManager const& unit_manager, GoodManager const& good_manager, ast::NodeCPtr root
		);
		bool load_pop_into_vector(
			RebelManager const& rebel_manager, std::vector<Pop>& vec, PopType const& type, ast::NodeCPtr pop_node,
			bool *non_integer_size
		) const;
	};
}
