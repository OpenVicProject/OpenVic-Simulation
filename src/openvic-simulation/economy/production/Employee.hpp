#pragma once

#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Employee {
	private:
		Pop::pop_size_t PROPERTY_RW(size);
	public:
		Pop& pop;
		Employee(Pop& new_pop, const Pop::pop_size_t new_size);
	};
}