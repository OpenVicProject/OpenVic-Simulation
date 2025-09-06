#pragma once

#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Getters.hpp"

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
