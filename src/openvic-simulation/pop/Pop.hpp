#pragma once

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

	private:
		const enum class strata_t {
			POOR,
			MIDDLE,
			RICH
		} strata;
		const sprite_t sprite;
		const Pop::pop_size_t max_size, merge_max_size;
		const bool state_capital_only, demote_migrant, is_artisan, is_slave;

		// TODO - rebel composition, life/everyday/luxury needs, country and province migration targets, promote_to targets, ideologies and issues

		PopType(std::string_view new_identifier, colour_t new_colour, strata_t new_strata, sprite_t new_sprite, Pop::pop_size_t new_max_size, Pop::pop_size_t new_merge_max_size,
			bool new_state_capital_only, bool new_demote_migrant, bool new_is_artisan, bool new_is_slave);

	public:
		PopType(PopType&&) = default;

		strata_t get_strata() const;
		sprite_t get_sprite() const;
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

		CultureManager& get_culture_manager();
		CultureManager const& get_culture_manager() const;
		ReligionManager& get_religion_manager();
		ReligionManager const& get_religion_manager() const;

		bool add_pop_type(std::string_view identifier, colour_t new_colour, PopType::strata_t strata, PopType::sprite_t sprite,
			Pop::pop_size_t max_size, Pop::pop_size_t merge_max_size, bool state_capital_only, bool demote_migrant,
			bool is_artisan, bool is_slave);
		IDENTIFIER_REGISTRY_ACCESSORS(pop_type)

		bool load_pop_type_file(std::string_view filestem, ast::NodeCPtr root);
		bool load_pop_into_province(Province& province, std::string_view pop_type_identifier, ast::NodeCPtr pop_node) const;
	};
}
