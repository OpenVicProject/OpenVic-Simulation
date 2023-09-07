#pragma once

#include "openvic/map/Province.hpp"

namespace OpenVic {

	struct ProvinceSet {
	protected:
		std::vector<Province*> provinces;
		bool locked = false;

	public:
		bool add_province(Province* province);
		void lock(bool log = false);
		bool is_locked() const;
		void reset();
		bool empty() const;
		size_t size() const;
		void reserve(size_t size);
		bool contains_province(Province const* province) const;
		std::vector<Province*> const& get_provinces() const;
	};

	/* REQUIREMENTS:
	 * MAP-6, MAP-44, MAP-48
	 */
	struct Region : HasIdentifier, ProvinceSet {
		friend struct Map;

	private:
		Region(const std::string_view new_identifier);

	public:
		Region(Region&&) = default;

		colour_t get_colour() const;
	};
}
