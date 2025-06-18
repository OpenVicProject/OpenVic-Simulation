#pragma once

#include <ranges>
#include <string_view>
#include <vector>

#include "openvic-simulation/modifier/Modifier.hpp"

namespace OpenVic {

	struct ProvinceDefinition;

	struct ProvinceSet {
	private:
		std::vector<ProvinceDefinition const*> PROPERTY(provinces);
		bool locked = false;

	public:
		/* Returns true if the province is successfully added, false if not (including if it's already in the set). */
		bool add_province(ProvinceDefinition const* province);

		template<std::ranges::sized_range Container>
		requires std::convertible_to<std::ranges::range_value_t<Container>, ProvinceDefinition const*>
		bool add_provinces(Container const& new_provinces) {
			reserve_more(new_provinces.size());

			bool ret = true;

			for (ProvinceDefinition const* province : new_provinces) {
				ret &= add_province(province);
			}

			return ret;
		}

		/* Returns true if the province is successfully removed, false if not (including if it's not in the set). */
		bool remove_province(ProvinceDefinition const* province);
		void lock(bool log = false);
		bool is_locked() const;
		void reset();
		bool empty() const;
		size_t size() const;
		size_t capacity() const;
		void reserve(size_t size);
		void reserve_more(size_t size);
		bool contains_province(ProvinceDefinition const* province) const;
	};

	struct ProvinceSetModifier : Modifier, ProvinceSet {
		friend struct MapDefinition;
	private:
		ProvinceSetModifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type);
	public:
		ProvinceSetModifier(ProvinceSetModifier&&) = default;
	};

	/* REQUIREMENTS:
	 * MAP-6, MAP-44, MAP-48
	 */
	struct Region : HasIdentifierAndColour, ProvinceSet {
		friend struct MapDefinition;

	private:
		/* A meta region cannot be the template for a state.
		 * Any region without provinces or containing water provinces is considered a meta region.
		 */
		const bool PROPERTY(is_meta);

		Region(std::string_view new_identifier, colour_t new_colour, bool new_is_meta);

	public:
		Region(Region&&) = default;
	};
}
