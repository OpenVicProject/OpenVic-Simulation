#include "Employee.hpp"

using namespace OpenVic;

Employee::Employee(Pop& new_pop, Pop::pop_size_t const new_size)
: pop { new_pop },
size { new_size }
{}