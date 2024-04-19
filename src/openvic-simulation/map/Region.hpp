#pragma once

#include "openvic-simulation/map/Province.hpp"

namespace OpenVic {

	struct ProvinceSet {
		using provinces_t = std::vector<Province const*>;

	private:
		provinces_t provinces;
		bool locked = false;

	public:
		/* Returns true if the province is successfully added, false if not (including if it's already in the set). */
		bool add_province(Province const* province);
		bool add_provinces(provinces_t const& new_provinces);
		/* Returns true if the province is successfully removed, false if not (including if it's not in the set). */
		bool remove_province(Province const* province);
		void lock(bool log = false);
		bool is_locked() const;
		void reset();
		bool empty() const;
		size_t size() const;
		void reserve(size_t size);
		void reserve_more(size_t size);
		bool contains_province(Province const* province) const;
		provinces_t const& get_provinces() const;
		Pop::pop_size_t calculate_total_population() const;
	};

	struct ProvinceSetModifier : Modifier, ProvinceSet {
		friend struct Map;
	private:
		ProvinceSetModifier(std::string_view new_identifier, ModifierValue&& new_values);
	public:
		ProvinceSetModifier(ProvinceSetModifier&&) = default;
	};

	/* REQUIREMENTS:
	 * MAP-6, MAP-44, MAP-48
	 */
	struct Region : HasIdentifierAndColour, ProvinceSet {
		friend struct Map;

	private:
		/* A meta region cannot be the template for a state.
		 * Any region containing a province already listed in a
		 * previously defined region is considered a meta region.
		 */
		const bool PROPERTY(meta);

		Region(std::string_view new_identifier, colour_t new_colour, bool new_meta);

	public:
		Region(Region&&) = default;
	};
}
