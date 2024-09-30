#pragma once

#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Employee {
	private:
		Pop::pop_size_t PROPERTY_RW(size);
	public:
		Pop& pop;
		Employee(Pop& new_pop, Pop::pop_size_t const new_size);
	};
}