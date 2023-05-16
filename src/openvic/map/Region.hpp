#pragma once

#include "Province.hpp"

namespace OpenVic {

	struct ProvinceSet {
	protected:
		std::vector<Province*> provinces;
	public:
		size_t get_province_count() const;
		bool contains_province(Province const* province) const;
		std::vector<Province*> const& get_provinces() const;
	};

	/* REQUIREMENTS:
	 * MAP-6, MAP-44, MAP-48
	 */
	struct Region : HasIdentifier, ProvinceSet {
		friend struct Map;
	private:
		Region(std::string const& new_identifier);
	public:
		Region(Region&&) = default;

		colour_t get_colour() const;
	};
}
