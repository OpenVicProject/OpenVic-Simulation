#pragma once

#include "openvic-simulation/map/Province.hpp"

namespace OpenVic {

	struct ProvinceSet {
		using provinces_t = std::vector<Province*>;

	private:
		provinces_t provinces;
		bool locked = false;

	public:
		ProvinceSet(provinces_t&& new_provinces = {});

		bool add_province(Province* province);
		void lock(bool log = false);
		bool is_locked() const;
		void reset();
		bool empty() const;
		size_t size() const;
		void reserve(size_t size);
		bool contains_province(Province const* province) const;
		provinces_t const& get_provinces() const;
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
		const bool meta;

		Region(std::string_view new_identifier, provinces_t&& new_provinces, bool new_meta);

	public:
		Region(Region&&) = default;

		bool get_meta() const;
	};
}
