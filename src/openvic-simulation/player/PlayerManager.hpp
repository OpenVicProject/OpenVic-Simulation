#pragma once

#include "openvic-simulation/core/Property.hpp"
#include "openvic-simulation/core/memory/String.hpp"

namespace OpenVic {
	struct CountryInstance;

	struct PlayerManager {
	private:
		memory::string PROPERTY(name);
		CountryInstance* PROPERTY_PTR(country, nullptr);

	public:
		void set_country(CountryInstance* instance);
	};
}
