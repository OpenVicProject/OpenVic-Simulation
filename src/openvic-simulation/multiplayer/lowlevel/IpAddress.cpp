#include "IpAddress.hpp"

#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

memory::string IpAddress::to_string(bool prefer_ipv4, to_chars_option option) const {
	stack_string result = to_array(prefer_ipv4, option);
	if (OV_unlikely(result.empty())) {
		return {};
	}

	return result;
}

IpAddress::operator memory::string() const {
	return to_string();
}
