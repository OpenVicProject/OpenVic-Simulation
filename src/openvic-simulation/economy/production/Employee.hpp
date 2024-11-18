#pragma once

#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Employee {
	private:
		pop_size_t PROPERTY_RW(size);
	public:
		Pop& pop;
		Employee(Pop& new_pop, const pop_size_t new_size);
	};
}